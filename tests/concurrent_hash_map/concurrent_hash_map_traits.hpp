// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * concurrent_hash_map_traits.hpp -- traits for testing
 * pmem::obj::concurrent_hash_map
 */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <thread>
#include <vector>

namespace nvobj = pmem::obj;

template <typename RootType, typename MapType>
struct ConcurrentHashMapTestPrimitives {
private:
	nvobj::pool<RootType> &m_pop;
	nvobj::persistent_ptr<MapType> map;
	size_t m_items_number;
	const size_t rehash_bucket_ratio = 8;

public:
	using value_type = typename MapType::value_type;

	ConcurrentHashMapTestPrimitives(nvobj::pool<RootType> &pop,
					nvobj::persistent_ptr<MapType> &map_ptr,
					size_t items_number)
	    : m_pop(pop), map(map_ptr), m_items_number(items_number)
	{
		map->runtime_initialize();
	}

	void
	reinitialize()
	{
		reinitialize(m_items_number);
	}

	void
	reinitialize(size_t expected)
	{
		size_t buckets = map->bucket_count();
		map->runtime_initialize();
		UT_ASSERT(map->bucket_count() == buckets);
		UT_ASSERT(map->size() == expected);
		map->runtime_initialize();
		UT_ASSERT(map->bucket_count() == buckets);
		UT_ASSERT(map->size() == expected);
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
		rehash(m_items_number);
	}

	void
	rehash(size_t expected)
	{
		map->rehash(m_items_number * rehash_bucket_ratio);
		check_items_count(expected);
	}

	template <typename AccessorType, typename Key, typename Obj>
	void
	check_item(Key i, Obj j)
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
		check_consistency(m_items_number);
	}

	void
	check_consistency(size_t expected)
	{
		check_items_count(expected);
		rehash(expected);
		reinitialize(expected);
	}

	void
	defragment()
	{
		map->defragment();
	}

	template <typename ItemType>
	void
	increment(ItemType i)
	{
		/* Do we need update method in cmap api? */
		typename MapType::accessor acc;
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
		typename MapType::accessor acc;
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
		typename MapType::accessor acc;
		bool ret = map->insert(acc, value_type(i, j));
		if (!ret) {
			/* Update needs to be persisted by the user */
			nvobj::transaction::run(
				m_pop, [&] { acc->second.get_rw()++; });
		}
	}

	void
	insert(std::initializer_list<value_type> il)
	{
		/* Initializer list insert is void type */
		map->insert(il);
		for (auto i : il) {
			auto key = i.first;
			UT_ASSERTeq(map->count(key), 1);
		}
	}

	void
	insert(std::vector<value_type> v)
	{
		/* Iterator insert is void type */
		map->insert(v.begin(), v.end());
		for (auto i : v) {
			auto key = i.first;
			UT_ASSERTeq(map->count(key), 1);
		}
	}

	template <typename K, typename M>
	bool
	insert_or_assign(K &&key, M &&obj)
	{
		return map->insert_or_assign(std::forward<K>(key),
					     std::forward<M>(obj));
	}
};
