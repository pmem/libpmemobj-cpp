/*
 * Copyright 2018-2019, Intel Corporation
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

#include "unittest.hpp"

#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
#include "tbb/spin_rw_mutex.h"
#endif

#include <iostream>
#include <thread>
#include <vector>

#include <libpmemobj++/container/concurrent_hash_map.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
typedef nvobj::concurrent_hash_map<
	nvobj::p<int>, nvobj::p<int>, std::hash<nvobj::p<int>>,
	std::equal_to<nvobj::p<int>>,
	pmem::obj::experimental::v<tbb::spin_rw_mutex>,
	tbb::spin_rw_mutex::scoped_lock>
	persistent_map_type;
#else
typedef nvobj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;
#endif
struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

template <typename Function>
void
parallel_exec(size_t concurrency, Function f)
{
	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back(f, i);
	}

	for (auto &t : threads) {
		t.join();
	}
}

struct ConcurrentHashMapTestPrimitives {
private:
	nvobj::pool<root> &m_pop;
	nvobj::persistent_ptr<persistent_map_type> map;
	size_t m_items_number;
	const size_t rehash_bucket_ratio = 8;

public:
	ConcurrentHashMapTestPrimitives(nvobj::pool<root> &pop,
					size_t items_number)
	    : m_pop(pop), m_items_number(items_number)
	{
		map = m_pop.root()->cons;
		map->runtime_initialize();
	}

	void
	reinitialize()
	{
		size_t buckets = map->bucket_count();
		map->runtime_initialize(true);
		UT_ASSERT(map->bucket_count() == buckets);
		UT_ASSERT(map->size() == m_items_number);
		map->runtime_initialize();
		UT_ASSERT(map->bucket_count() == buckets);
		UT_ASSERT(map->size() == m_items_number);
	}

	void
	check_items_count()
	{
		check_items_count(m_items_number);
	}

	void
	check_items_count(size_t expected)
	{
		UT_ASSERT(map->size() == expected);
		UT_ASSERT(std::distance(map->begin(), map->end()) ==
			  int(expected));
	}

	void
	clear()
	{
		map->clear();
		UT_ASSERT(map->size() == 0);
		UT_ASSERT(std::distance(map->begin(), map->end()) == 0);
	}

	void
	rehash()
	{
		map->rehash(m_items_number * rehash_bucket_ratio);
		check_items_count();
	}

	template <typename AccessorType, typename ItemType>
	void
	check_item(ItemType i, ItemType j)
	{
		AccessorType acc;
		bool found = map->find(acc, i);
		UT_ASSERT(found == true);
		UT_ASSERT(acc->first == i);
		UT_ASSERT(acc->second == j);
	}
	void
	check_consistency()
	{
		check_items_count();
		rehash();
		reinitialize();
	}

	template <typename ItemType>
	void
	increment(ItemType i)
	{
		/* Do we need update method in cmap api? */
		persistent_map_type::accessor acc;
		bool found = map->find(acc, i);
		UT_ASSERT(found == true);
		UT_ASSERT(acc->first == i);
		auto old_val = acc->second;
		acc->second.get_rw() += 1;
		m_pop.persist(acc->second);
		UT_ASSERT(acc->second == (old_val + 1));
	}

	template <typename ItemType>
	void
	erase(ItemType i)
	{
		bool res = map->erase(i);
		UT_ASSERT(res == true);
	}

	template <typename ItemType>
	void
	check_erased(ItemType i)
	{
		persistent_map_type::accessor acc;
		bool found = map->find(acc, i);
		UT_ASSERT(found == false);
	}

	template <typename ValueType>
	void
	insert(ValueType val)
	{
		bool ret = map->insert(val);
		UT_ASSERT(ret == true);
	}

	template <typename AccessorType, typename ValueType>
	void
	insert(ValueType val)
	{
		AccessorType accessor;
		bool ret = map->insert(accessor, val);
		UT_ASSERT(ret == true);
	}

	template <typename ItemType>
	void
	insert_or_increment(ItemType i, ItemType j)
	{
		persistent_map_type::accessor acc;
		bool ret =
			map->insert(acc, persistent_map_type::value_type(i, j));
		if (!ret) {
			/* Update needs to be persisted by the user */
			nvobj::transaction::run(
				m_pop, [&] { acc->second.get_rw()++; });
		}
	}

	void
	insert(std::initializer_list<persistent_map_type::value_type> il)
	{
		/* Initializer list insert is void type */
		map->insert(il);
		for (auto i : il) {
			auto key = i.first;
			UT_ASSERTeq(map->count(key), 1);
		}
	}

	void
	insert(std::vector<persistent_map_type::value_type> v)
	{
		/* Iterator insert is void type */
		map->insert(v.begin(), v.end());
		for (auto i : v) {
			auto key = i.first;
			UT_ASSERTeq(map->count(key), 1);
		}
	}
};

/*
 * insert_and_lookup_value_type_test -- test insert and lookup
 * Implements tests for:
 * bool pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert (const_accessor &result, const value_type &value)
 * bool pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert(accessor &result, const value_type &value)
 * bool pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert(const_accessor &result, value_type &&value)
 * bool pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert(accessor &result, value_type &&value)
 *  All find()
 *  Update element
 */
template <typename InsertAccessor, typename ValueType>
void
insert_and_lookup_value_type_test(nvobj::pool<root> &pop,
				  size_t concurrency = 8,
				  size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;
	ConcurrentHashMapTestPrimitives test(pop, concurrency * thread_items);

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		for (int i = begin; i < end; i++) {
			auto val = ValueType(i, i);
			test.insert<InsertAccessor>(val);
		}
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::accessor>(i, i);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::const_accessor>(i,
									     i);
		for (int i = begin; i < end; i++)
			test.increment(i);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::accessor>(i,
								       i + 1);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::const_accessor>(
				i, i + 1);
	});
	test.check_consistency();
	test.clear();
}

/*
 * insert_and_lookup_value_type_test -- test insert and lookup
 * Implements tests for:
 * bool pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert(const value_type &value)
 * bool pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert(const value_type &value)
 * All find()
 * Update element
 */
template <typename InsertAccessor, typename ValueType>
void
insert_and_lookup_key_test(nvobj::pool<root> &pop, size_t concurrency = 8,
			   size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;
	ConcurrentHashMapTestPrimitives test(pop, concurrency * thread_items);

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		for (int i = begin; i < end; i++) {
			auto val = ValueType(i);
			test.insert<InsertAccessor>(val);
		}
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::accessor>(i, 0);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::const_accessor>(i,
									     0);
		for (int i = begin; i < end; i++)
			test.increment(i);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::accessor>(i, 1);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::const_accessor>(i,
									     1);
	});
	test.check_consistency();
	test.clear();
}

/*
 * insert_and_lookup_value_type_test -- test insert and lookup
 * Implements tests for:
 * bool pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert(const value_type &value)
 * bool pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::inserti(value_type &&value)
 */
template <typename ValueType>
void
insert_and_lookup_value_type_test(nvobj::pool<root> &pop,
				  size_t concurrency = 8,
				  size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;
	ConcurrentHashMapTestPrimitives test(pop, concurrency * thread_items);

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		for (int i = begin; i < end; i++) {
			auto val = ValueType(i, i);
			test.insert(val);
		}
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::accessor>(i, i);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::const_accessor>(i,
									     i);
		for (int i = begin; i < end; i++)
			test.increment(i);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::accessor>(i,
								       i + 1);
		for (int i = begin; i < end; i++)
			test.check_item<persistent_map_type::const_accessor>(
				i, i + 1);
	});
	test.check_consistency();
	test.clear();
}

/*
 * insert_and_lookup_initializer_list -- test insert elements using
 * initializer_list and lookup operations. This tests inserts only two keys, due
 * to syntax limitations of initializer_list
 * Implements tests for:
 * pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	 KeyEqual>::insert(std::initializer_list< value_type > il)
 */
void
insert_and_lookup_initializer_list_test(nvobj::pool<root> &pop,
					size_t concurrency = 8)
{
	PRINT_TEST_PARAMS;
	ConcurrentHashMapTestPrimitives test(pop, concurrency * 2);

	parallel_exec(concurrency, [&](size_t thread_id) {
		auto k1 =
			persistent_map_type::value_type(int(thread_id), 0xDEAD);
		/* User have to avoid collisions manually*/
		auto k2 = persistent_map_type::value_type(
			int(concurrency) + int(thread_id), 0xBEEF);
		test.insert(
			std::initializer_list<persistent_map_type::value_type>{
				k1, k2});
		test.check_item<persistent_map_type::accessor>(k1.first,
							       k1.second);
		test.check_item<persistent_map_type::const_accessor>(k2.first,
								     k2.second);
	});
	test.check_consistency();
	test.clear();
}

/*
 * insert_and_lookup_iterator_test -- test insert and lookup
 * Implements tests for:
 * void pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert(I first, I last)
 */
void
insert_and_lookup_iterator_test(nvobj::pool<root> &pop, size_t concurrency = 8,
				size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;

	ConcurrentHashMapTestPrimitives test(pop, concurrency * thread_items);
	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		std::vector<persistent_map_type::value_type> v;
		for (int i = begin; i < end; ++i) {
			v.push_back(persistent_map_type::value_type(i, i));
		}
		test.insert(v);
		for (auto i : v)
			test.check_item<persistent_map_type::accessor>(
				i.first, i.second);
	});
	test.check_consistency();
	test.clear();
}

/*
 * insert_mt_test -- test insert for small number of elements
 * Implements tests for:
 * void pmem::obj::concurrent_hash_map< Key, T, Hash,
 *	KeyEqual>::insert(I first, I last)
 */
void
insert_mt_test(nvobj::pool<root> &pop, size_t concurrency = 8,
	       size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;

	ConcurrentHashMapTestPrimitives test(pop, thread_items);
	parallel_exec(concurrency, [&](size_t thread_id) {
		for (int i = 0; i < int(thread_items); i++) {
			test.insert_or_increment(i, 1);
		}
	});
	for (int i = 0; i < int(thread_items); i++) {
		test.check_item<persistent_map_type::const_accessor>(
			i, (int)concurrency);
	}
	test.check_consistency();
	for (int i = 0; i < int(thread_items); i++) {
		test.check_item<persistent_map_type::accessor>(
			i, (int)concurrency);
	}
	test.clear();
}

/*
 * insert_and_erase_test -- (internal) test insert and erase operations
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */

template <typename InsertAccessor, typename ValueType>
void
insert_and_erase_test(nvobj::pool<root> &pop, size_t concurrency = 8,
		      size_t thread_items = 50)
{

	PRINT_TEST_PARAMS;

	ConcurrentHashMapTestPrimitives test(pop, concurrency * thread_items);

	// Adding more concurrency will increase DRD test time
	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * thread_items;
		int end = begin + int(thread_items);
		for (int i = begin; i < end; ++i) {
			ValueType val(i, i);
			test.insert<InsertAccessor>(val);
		}
		for (int i = begin; i < end; ++i) {
			test.erase(i);
			test.check_erased(i);
		}
	});
	test.check_items_count(0);
	test.clear();
}

/*
 * insert_and_erase_test -- (internal) test insert and erase operations
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_erase_lookup_test(nvobj::pool<root> &pop, size_t concurrency = 4)
{

	PRINT_TEST_PARAMS;
	const size_t NUMBER_ITEMS_INSERT = 50;

	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				map->insert(
					persistent_map_type::value_type(i, i));
			}
		});
	}

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				map->erase(i);
			}
		});
	}

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				persistent_map_type::accessor acc;
				bool res = map->find(acc, i);

				if (res) {
					UT_ASSERTeq(acc->first, i);
					UT_ASSERT(acc->second >= i);
					acc->second.get_rw() += 1;
					pop.persist(acc->second);
				}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	for (auto &e : *map) {
		UT_ASSERT(e.first <= e.second);
	}
}
