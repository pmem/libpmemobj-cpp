/*
 * Copyright 2019-2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * A volatile state for persistent objects.
 *
 * This feature requires C++14 support.
 */

#ifndef LIBPMEMOBJ_CPP_VOLATILE_STATE_HPP
#define LIBPMEMOBJ_CPP_VOLATILE_STATE_HPP

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <tuple>
#include <unordered_map>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem
{

namespace detail
{

/**
 * Global key value store which allows persistent objects to
 * use volatile memory. Entries in this kv store are indexed
 * by PMEMoids.
 */
class volatile_state {
public:
	template <typename T>
	static T *
	get_if_exists(const PMEMoid &oid)
	{
		auto &map = get_map();

		{
			std::shared_lock<rwlock_type> lock(get_rwlock());
			auto it = map.find(oid);
			if (it != map.end())
				return static_cast<T *>(it->second.get());
			else
				return nullptr;
		}
	}

	template <typename T>
	static T *
	get(const PMEMoid &oid)
	{
		auto &map = get_map();

		auto element = get_if_exists<T>(oid);
		if (element)
			return element;

		if (pmemobj_tx_stage() != TX_STAGE_NONE)
			throw pmem::transaction_scope_error(
				"volatile_state::get() cannot be called in a transaction");

		{
			std::unique_lock<rwlock_type> lock(get_rwlock());

			auto deleter = [](void const *data) {
				T const *p = static_cast<T const *>(data);
				delete p;
			};

			auto it = map.find(oid);
			if (it == map.end()) {
				auto ret = map.emplace(
					std::piecewise_construct,
					std::forward_as_tuple(oid),
					std::forward_as_tuple(new T, deleter));

				/* emplace could fail only if there is already
				 * an element with the same key which is not
				 * possible */
				assert(ret.second);

				it = ret.first;

				auto pop = pmemobj_pool_by_oid(oid);
				auto *user_data =
					static_cast<detail::pool_data *>(
						pmemobj_get_user_data(pop));

				user_data->set_cleanup([oid] {
					clear_from_pool(oid.pool_uuid_lo);
				});
			}

			return static_cast<T *>(it->second.get());
		}
	}

	static void
	destroy(const PMEMoid &oid)
	{
		if (pmemobj_tx_stage() == TX_STAGE_WORK) {
			obj::transaction::register_callback(
				obj::transaction::stage::oncommit, [oid] {
					std::unique_lock<rwlock_type> lock(
						get_rwlock());
					get_map().erase(oid);
				});
		} else {
			std::unique_lock<rwlock_type> lock(get_rwlock());
			get_map().erase(oid);
		}
	}

private:
	struct pmemoid_hash {
		std::size_t
		operator()(const PMEMoid &oid) const
		{
			return oid.pool_uuid_lo + oid.off;
		}
	};

	struct pmemoid_equal_to {
		bool
		operator()(const PMEMoid &lhs, const PMEMoid &rhs) const
		{
			return lhs.pool_uuid_lo == rhs.pool_uuid_lo &&
				lhs.off == rhs.off;
		}
	};

	using key_type = PMEMoid;
	using value_type =
		std::unique_ptr<void,
				std::add_pointer<void(const void *)>::type>;

	using map_type = std::unordered_map<key_type, value_type, pmemoid_hash,
					    pmemoid_equal_to>;

	using rwlock_type = std::shared_timed_mutex;

	static void
	clear_from_pool(uint64_t pool_id)
	{
		std::unique_lock<rwlock_type> lock(get_rwlock());
		auto &map = get_map();

		for (auto it = map.begin(); it != map.end();) {
			if (it->first.pool_uuid_lo == pool_id)
				it = map.erase(it);
			else
				++it;
		}
	}

	static map_type &
	get_map()
	{
		static map_type map;
		return map;
	}

	static rwlock_type &
	get_rwlock()
	{
		static rwlock_type rwlock;
		return rwlock;
	}
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_VOLATILE_STATE_HPP */
