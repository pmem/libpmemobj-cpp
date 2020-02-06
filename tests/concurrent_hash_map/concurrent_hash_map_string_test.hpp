// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * concurrent_hash_map_string_test.hpp -- pmem::obj::concurrent_hash_map test
 */

#include "concurrent_hash_map_traits.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/container/vector.hpp>
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
typedef nvobj::vector<nvobj::string> tls_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
	nvobj::persistent_ptr<tls_type> tls;
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

	nvobj::persistent_ptr<tls_type> &tls = pop.root()->tls;
	using accessor = persistent_map_type::accessor;
	using const_acc = persistent_map_type::const_accessor;

	tls->resize(concurrency);
	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		auto &pstr = tls->at(thread_id);

		for (int i = begin; i < end; i++) {
			pstr = std::to_string(i);
			const persistent_map_type::key_type &val = pstr;
			bool result = test.insert_or_assign(val, i);
			UT_ASSERT(result);
		}
		for (int i = begin; i < end; i++) {
			test.check_item<accessor>(std::to_string(i), i);
		}
		for (int i = begin; i < end; i++) {
			/* assign existing keys new values */
			pstr = std::to_string(i);
			const persistent_map_type::key_type &val = pstr;
			bool result = test.insert_or_assign(val, i + 1);
			UT_ASSERT(!result);
		}
		for (int i = begin; i < end; i++) {
			test.check_item<const_acc>(std::to_string(i), i + 1);
		}
	});
	test.check_consistency();
	test.clear();
	tls->clear();
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

	nvobj::persistent_ptr<tls_type> &tls = pop.root()->tls;
	using accessor = persistent_map_type::accessor;
	using const_acc = persistent_map_type::const_accessor;

	tls->resize(concurrency);
	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		auto &pstr = tls->at(thread_id);

		for (int i = begin; i < end; i++) {
			pstr = std::to_string(i);
			bool result = test.insert_or_assign(std::move(pstr), i);
			UT_ASSERT(result);
		}
		for (int i = begin; i < end; i++) {
			test.check_item<accessor>(std::to_string(i), i);
		}
		for (int i = begin; i < end; i++) {
			/* assign existing keys new values */
			pstr = std::to_string(i);
			bool result =
				test.insert_or_assign(std::move(pstr), i + 1);
			UT_ASSERT(!result);
		}
		for (int i = begin; i < end; i++) {
			test.check_item<const_acc>(std::to_string(i), i + 1);
		}
	});
	test.check_consistency();
	test.clear();
	tls->clear();
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
	using const_acc = persistent_map_type::const_accessor;

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);

		for (int i = begin; i < end; i++) {
			bool result =
				test.insert_or_assign(std::to_string(i), i);
			UT_ASSERT(result);
		}
		for (int i = begin; i < end; i++) {
			test.check_item<accessor>(std::to_string(i), i);
		}
		for (int i = begin; i < end; i++) {
			/* assign existing keys new values */
			bool result =
				test.insert_or_assign(std::to_string(i), i + 1);
			UT_ASSERT(!result);
		}
		for (int i = begin; i < end; i++) {
			test.check_item<const_acc>(std::to_string(i), i + 1);
		}
	});
	test.check_consistency();
	test.clear();
}
