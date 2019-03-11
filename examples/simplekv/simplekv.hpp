/*
 * Copyright 2019, Intel Corporation
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

#include <bitset>
#include <iostream>
#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <stdexcept>
#include <string>

namespace examples
{

namespace ptl = pmem::obj::experimental;

using pmem::obj::delete_persistent;
using pmem::obj::make_persistent;
using pmem::obj::mutex;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::pool_base;
using pmem::obj::transaction;

template <typename Key, typename Value, typename HashFunc, std::size_t N>
class kv {
private:
	struct slot;
	struct entry;

	static const int nretries = 5;
	static const int nhash = 2;

	ptl::array<slot, N> slots[nhash];
	ptl::vector<entry> entries;

public:
	using value_type = entry;

	kv() = default;

	Value &
	at(const Key &k)
	{
		for (int n = 0; n < nhash; ++n) {
			auto slot = slots[n].const_at(key_hash(k, n));
			const auto &entry = entries.const_at(slot.index);
			if (entry.key == k)
				return entries.at(slot.index).value;
		}

		throw std::out_of_range("no entry in simplekv");
	}

	bool
	exists(const Key &k)
	{
		try {
			at(k);
			return true;
		} catch (std::out_of_range) {
			return false;
		}
	}

	void
	insert(const Key &k, const Value &v)
	{
		auto pop = get_pool();

		/* try to insert element in on of nhash positions */
		for (int hash_func = 0; hash_func < nhash; hash_func++) {
			auto key_index = key_hash(k, hash_func);
			const auto &cslot =
				slots[hash_func].const_at(key_index);

			/* element with provided key is already present */
			if (cslot.is_occupied() &&
			    entries.const_at(cslot.index).key == k) {
				throw std::runtime_error(
					"entry already exists");
			} else if (!cslot.is_occupied()) {

				transaction::run(pop, [&] {
					entries.emplace_back(k, v);
					slots[hash_func].at(key_index).set(
						entries.size() - 1);
				});

				return;
			}
		}

		/* all slots are occupied, try to displace an element */
		int retries = 0;
		transaction::run(pop, [&] {
			entries.emplace_back(k, v);

			auto hash_func = 0;
			auto key_index = key_hash(k, hash_func);
			auto displaced = slots[hash_func].const_at(key_index);

			slots[hash_func].at(key_index).set(entries.size() - 1);

			do {
				retries++;
				/* we hit retry threshold - abort */
				if (retries >= nretries) {
					/* exception will abort the transaction
					 * and rollback all the changes */
					throw std::runtime_error(
						"too many retries");
				}

				hash_func = (hash_func + 1) % nhash;
				key_index = key_hash(
					entries.const_at(displaced.index).key,
					hash_func);

				std::swap(slots[hash_func].at(key_index),
					  displaced);
			} while (displaced.is_occupied());
		});
	}

	auto
	begin() -> decltype(entries.begin())
	{
		return entries.begin();
	}

	auto
	end() -> decltype(entries.end())
	{
		return entries.end();
	}

	auto
	begin() const -> decltype(entries.cbegin())
	{
		return entries.cbegin();
	}

	auto
	end() const -> decltype(entries.cend())
	{
		return entries.cend();
	}

	auto
	cbegin() const -> decltype(entries.cbegin())
	{
		return entries.cbegin();
	}

	auto
	cend() const -> decltype(entries.cend())
	{
		return entries.cend();
	}

private:
	size_t
	key_hash(const Key &k, int n) const
	{
		return HashFunc{}(k, n) & (N - 1);
	}

	pool_base
	get_pool() const noexcept
	{
		auto pop = pmemobj_pool_by_ptr(this);
		assert(pop != nullptr);
		return pool_base(pop);
	}

	struct entry {
		entry(const Key &k = 0, const Value &v = Value{})
		    : key(k), value(v)
		{
		}

		Key key;
		Value value;
	};

	struct slot {
		enum flag {
			occupied,
		};

		slot() : flags(0), index(0)
		{
		}

		bool
		is_occupied() const
		{
			return flags[slot::flag::occupied];
		}

		void
		clear()
		{
			flags[slot::flag::occupied] = false;
		}

		void
		set(uint32_t index)
		{
			this->index = index;

			flags[slot::flag::occupied] = true;
		}

		std::bitset<1> flags;
		uint32_t index;
	};
};

} /* namespace examples */
