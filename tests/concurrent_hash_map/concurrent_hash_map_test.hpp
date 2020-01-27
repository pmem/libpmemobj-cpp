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

#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
#include "tbb/spin_rw_mutex.h"
#endif

#include <chrono>
#include <condition_variable>
#include <mutex>

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
	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, concurrency * thread_items);

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
	test.defragment();
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
	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, concurrency * thread_items);

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
	test.defragment();
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
	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, concurrency * thread_items);

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
	test.defragment();
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
	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, concurrency * 2);

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
	test.defragment();
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

	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, concurrency * thread_items);
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
	test.defragment();
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

	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, thread_items);
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
	test.check_consistency();
	test.defragment();
	test.check_consistency();
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

	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, concurrency * thread_items);

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
	test.defragment();
	test.check_consistency(0);
	test.clear();
}

/*
 * insert_erase_count_test -- (internal) test insert and erase operations
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_erase_count_test(nvobj::pool<root> &pop, size_t concurrency = 8,
		      size_t thread_items = 50)
{
	PRINT_TEST_PARAMS;

	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, concurrency * thread_items);

	// Adding more concurrency will increase DRD test time
	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	size_t n_erased = 0;

	parallel_exec(2, [&](size_t thread_id) {
		if (thread_id == 0) {
			for (int i = 0; i < int(concurrency * thread_items);
				++i) {
				persistent_map_type::value_type val(i, i);
				test.insert(val);
			}
		} else {
			for (int i = int(concurrency * thread_items - 1);
				i >= 0; i--) {
				auto ret = map->erase(i);
				n_erased += (size_t)ret;
			}
		}
	});

	map->runtime_initialize();

	test.check_items_count(concurrency * thread_items - n_erased);
	test.clear();

	for (int i = 0; i < int(concurrency * thread_items); ++i) {
		persistent_map_type::value_type val(i, i);
		test.insert(val);
	}

	test.check_items_count(concurrency * thread_items);

	/* Use non-main thread */
	parallel_exec(1, [&](size_t thread_id) {
		for (int i = 0; i < int(concurrency * thread_items); ++i) {
			test.erase(i);
		}
	});

	test.check_items_count(0);

	map->runtime_initialize();

	test.check_items_count(0);
}

/*
 * insert_and_erase_test -- (internal) test insert and erase operations
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_erase_lookup_test(nvobj::pool<root> &pop, size_t concurrency = 4,
			int defrag = 0)
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

	if (defrag) {
		threads.emplace_back([&]() {
			map->defragment();
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	for (auto &e : *map) {
		UT_ASSERT(e.first <= e.second);
	}
}

/*
 * insert_erase_deadlock_test -- (internal) test lookup and erase operations
 * to the same bucket while holding an accessor to item (in the same bucket)
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
lookup_insert_erase_deadlock_test(nvobj::pool<root> &pop)
{
	PRINT_TEST_PARAMS;

	/* All elements will go to the same bucket. */
	static constexpr int elements[] = {1, 257, 513};

	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, sizeof(elements) / sizeof(elements[0]));

	auto map = pop.root()->cons;
	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	for (auto &e : elements)
		map->insert(persistent_map_type::value_type(e, e));

	std::condition_variable cv;
	bool ready = false;
	std::mutex m;

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	VALGRIND_HG_DISABLE_CHECKING(&m, sizeof(m));
#endif

	auto lookup_thread = [&] {
		persistent_map_type::accessor acc1;
		map->find(acc1, elements[0]);

		{
			std::unique_lock<std::mutex> lock(m);
			ready = true;
			cv.notify_one();
		}

		/* Wait until other thread calls erase() */
		{
			std::this_thread::sleep_for(std::chrono::seconds{1});
		}

		persistent_map_type::accessor acc2;
		map->find(acc2, elements[1]);
	};

	auto erase_thread = [&] {
		{
			std::unique_lock<std::mutex> lock(m);
			cv.wait(lock, [&] { return ready; });
		}

		/* Test erase of element to which is locked by other thread */
		map->erase(elements[0]);
	};

	auto lookup_insert_thread = [&] {
		{
			std::unique_lock<std::mutex> lock(m);
			cv.wait(lock, [&] { return ready; });
		}

		persistent_map_type::accessor acc1;
		map->find(acc1, elements[0]);

		persistent_map_type::accessor acc2;
		map->find(acc2, elements[2]);

		persistent_map_type::accessor acc3;
		map->insert(acc3, persistent_map_type::value_type(1025, 1025));
	};

	parallel_exec(2, [&](size_t tid) {
		if (tid == 0)
			lookup_thread();
		else
			erase_thread();
	});

	ready = false;
	parallel_exec(2, [&](size_t tid) {
		if (tid == 0)
			lookup_thread();
		else
			lookup_insert_thread();
	});
}
