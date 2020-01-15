/*
 * Copyright 2018-2020, Intel Corporation
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

/*
 * concurrent_hash_map_test.hpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "concurrent_hash_map_traits.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <string>

#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
#include "tbb/spin_rw_mutex.h"
#endif

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

class key_equal {
public:
	template <typename M, typename U>
	bool
	operator()(const M &lhs, const U &rhs) const
	{
		return lhs == rhs;
	}
};

class string_hasher {
	/* hash multiplier used by fibonacci hashing */
	static const size_t hash_multiplier = 11400714819323198485ULL;

public:
	using transparent_key_equal = key_equal;

	size_t
	operator()(const nvobj::string &str) const
	{
		return hash(str.c_str(), str.size());
	}

	size_t
	operator()(const std::string &str) const
	{
		return hash(str.c_str(), str.size());
	}

private:
	size_t
	hash(const char *str, size_t size) const
	{
		size_t h = 0;
		for (size_t i = 0; i < size; ++i) {
			h = static_cast<size_t>(str[i]) ^ (h * hash_multiplier);
		}
		return h;
	}
};

#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
typedef nvobj::concurrent_hash_map<nvobj::string, nvobj::p<int>, string_hasher,
				   std::equal_to<nvobj::string>,
				   nvobj::experimental::v<tbb::spin_rw_mutex>,
				   tbb::spin_rw_mutex::scoped_lock>
	persistent_map_type;
#else
typedef nvobj::concurrent_hash_map<nvobj::string, nvobj::p<int>, string_hasher>
	persistent_map_type;
#endif
struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

/*
 * insert_or_assign_lvalue -- test insert_or_assign with lvalue key
 * pmem::obj::concurrent_hash_map<nvobj::string, nvobj::string >
 */
void
insert_or_assign_lvalue(nvobj::pool<root> &pop, size_t concurrency = 8,
			size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;
	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, thread_items * concurrency);
	using accessor = persistent_map_type::accessor;
	using const_accessor = persistent_map_type::const_accessor;

	parallel_exec(concurrency, [&](size_t thread_id) {
		nvobj::persistent_ptr<nvobj::string> ptr;
		{
			pmem::obj::transaction::manual tx(pop);
			ptr = nvobj::make_persistent<nvobj::string>();
			pmem::obj::transaction::commit();
		}

		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		std::string val;
		bool result;

		for (int i = begin; i < end; i++) {
			ptr->assign(std::to_string(i));
			const persistent_map_type::key_type &val = *ptr;
			result = test.insert_or_assign(val, i);
			UT_ASSERT(result);
		}
		for (int i = begin; i < end; i++) {
			val = std::to_string(i);
			test.check_item<accessor>(val, i);
		}
		for (int i = begin; i < end; i++) {
			/* assign existing keys new values */
			ptr->assign(std::to_string(i));
			const persistent_map_type::key_type &val = *ptr;
			result = test.insert_or_assign(val, i + 1);
			UT_ASSERT(!result);
		}
		for (int i = begin; i < end; i++) {
			val = std::to_string(i);
			test.check_item<const_accessor>(val, i + 1);
		}
	});
	test.check_consistency();
	test.clear();
}

/*
 * insert_or_assign_rvalue -- test insert_or_assign with rvalue key
 * pmem::obj::concurrent_hash_map<nvobj::string, nvobj::string >
 */
void
insert_or_assign_rvalue(nvobj::pool<root> &pop, size_t concurrency = 8,
			size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;
	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, thread_items * concurrency);
	using accessor = persistent_map_type::accessor;
	using const_accessor = persistent_map_type::const_accessor;

	parallel_exec(concurrency, [&](size_t thread_id) {
		nvobj::persistent_ptr<nvobj::string> ptr;
		{
			pmem::obj::transaction::manual tx(pop);
			ptr = nvobj::make_persistent<nvobj::string>();
			pmem::obj::transaction::commit();
		}

		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		std::string val;
		bool result;

		for (int i = begin; i < end; i++) {
			ptr->assign(std::to_string(i));
			result = test.insert_or_assign(std::move(*ptr), i);
			UT_ASSERT(result);
		}
		for (int i = begin; i < end; i++) {
			val = std::to_string(i);
			test.check_item<accessor>(val, i);
		}
		for (int i = begin; i < end; i++) {
			/* assign existing keys new values */
			ptr->assign(std::to_string(i));
			result = test.insert_or_assign(std::move(*ptr), i + 1);
			UT_ASSERT(!result);
		}
		for (int i = begin; i < end; i++) {
			val = std::to_string(i);
			test.check_item<const_accessor>(val, i + 1);
		}
	});
	test.check_consistency();
	test.clear();
}

/*
 * insert_or_assign_heterogeneous -- test insert_or_assign with key-comparable
 * pmem::obj::concurrent_hash_map<nvobj::string, nvobj::string >
 */
void
insert_or_assign_heterogeneous(nvobj::pool<root> &pop, size_t concurrency = 8,
			       size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;
	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, thread_items * concurrency);
	using accessor = persistent_map_type::accessor;
	using const_accessor = persistent_map_type::const_accessor;

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		std::string val;
		bool result;

		for (int i = begin; i < end; i++) {
			val = std::to_string(i);
			result = test.insert_or_assign(val, i);
			UT_ASSERT(result);
		}
		for (int i = begin; i < end; i++) {
			val = std::to_string(i);
			test.check_item<accessor>(val, i);
		}
		for (int i = begin; i < end; i++) {
			/* assign existing keys new values */
			val = std::to_string(i);
			result = test.insert_or_assign(val, i + 1);
			UT_ASSERT(!result);
		}
		for (int i = begin; i < end; i++) {
			val = std::to_string(i);
			test.check_item<const_accessor>(val, i + 1);
		}
	});
	test.check_consistency();
	test.clear();
}
