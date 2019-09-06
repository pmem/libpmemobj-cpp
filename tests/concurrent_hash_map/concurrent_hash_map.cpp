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
 * concurrent_hash_map.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <thread>
#include <vector>

#include <libpmemobj++/experimental/concurrent_hash_map.hpp>
#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/experimental/vector.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

class key_equal {
public:
	bool
	operator()(const pmem::obj::experimental::string &str, int rhs)
	{
		return str == std::to_string(rhs);
	}

	bool
	operator()(int lhs, const pmem::obj::experimental::string &str)
	{
		return str == std::to_string(lhs);
	}

	bool
	operator()(const pmem::obj::experimental::string &str1,
		   const pmem::obj::experimental::string &str2)
	{
		return str1 == str2;
	}
};

class string_hasher {
	/* hash multiplier used by fibonacci hashing */
	static const size_t hash_multiplier = 11400714819323198485ULL;

public:
	using transparent_key_equal = key_equal;

	size_t
	operator()(const pmem::obj::experimental::string &str) const
	{
		return hash(str.c_str(), str.size());
	}

	size_t
	operator()(int i) const
	{
		auto str = std::to_string(i);
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

bool
operator==(const pmem::obj::experimental::string &str, int i)
{
	return str == std::to_string(i);
}

bool
operator>=(const pmem::obj::experimental::string &str, int i)
{
	return std::atoi(str.c_str()) >= i;
}

/* Override operator<= to perform integral comparison */
bool
operator<=(const pmem::obj::experimental::string &str1,
	   const pmem::obj::experimental::string &str2)
{
	return std::atoi(str1.c_str()) <= std::atoi(str2.c_str());
}

typedef nvobj::experimental::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type_int;

typedef nvobj::experimental::concurrent_hash_map<
	pmem::obj::experimental::string, pmem::obj::experimental::string,
	string_hasher, key_equal>
	persistent_map_type_string;

struct root {
	nvobj::persistent_ptr<persistent_map_type_int> cons1;
	nvobj::persistent_ptr<persistent_map_type_string> cons2;

	nvobj::persistent_ptr<pmem::obj::experimental::vector<
		persistent_map_type_string::value_type>>
		vec;
};

static size_t constexpr MAX_ITEMS = 500;

persistent_map_type_int::value_type
create_value(nvobj::pool<root> &, persistent_map_type_int &, int i)
{
	return persistent_map_type_int::value_type(i, i);
}

persistent_map_type_string::value_type &
create_value(nvobj::pool<root> &pop, persistent_map_type_string &, int i)
{
	assert(i < static_cast<int>(MAX_ITEMS));
	return (*pop.root()->vec)[static_cast<size_t>(i)];
}

void
increase_element(nvobj::pool<root> &pop, pmem::obj::experimental::string &str)
{
	auto i = std::atoi(str.c_str());
	str = std::to_string(i + 1);
}

void
increase_element(nvobj::pool<root> &pop, pmem::obj::p<int> &i)
{
	i.get_rw()++;
	pop.persist(i);
}

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

/*
 * insert_and_lookup_test -- (internal) test insert and lookup operations
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
template <typename MapType>
void
insert_and_lookup_test(nvobj::pool<root> &pop,
		       nvobj::persistent_ptr<MapType> &map)
{
	const size_t NUMBER_ITEMS_INSERT = 50;

	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 8;

	size_t TOTAL_ITEMS = NUMBER_ITEMS_INSERT * concurrency;

	UT_ASSERT(map != nullptr);

	map->initialize();

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * NUMBER_ITEMS_INSERT;
		int end = begin + int(NUMBER_ITEMS_INSERT);
		for (int i = begin; i < end; ++i) {
			bool ret = map->insert(create_value(pop, *map, i));
			UT_ASSERT(ret == true);

			UT_ASSERT(map->count(i) == 1);

			typename MapType::accessor acc;
			bool res = map->find(acc, i);
			UT_ASSERT(res == true);
			UT_ASSERT(acc->first == i);
			UT_ASSERT(acc->second == i);
			increase_element(pop, acc->second);
		}

		for (int i = begin; i < end; ++i) {
			typename MapType::const_accessor const_acc;
			bool res = map->find(const_acc, i);
			UT_ASSERT(res == true);
			UT_ASSERT(const_acc->first == i);
			UT_ASSERT(const_acc->second == i + 1);
		}
	});

	UT_ASSERT(map->size() == TOTAL_ITEMS);

	UT_ASSERT(std::distance(map->begin(), map->end()) == int(TOTAL_ITEMS));

	map->rehash(TOTAL_ITEMS * 8);

	UT_ASSERT(map->size() == TOTAL_ITEMS);

	UT_ASSERT(std::distance(map->begin(), map->end()) == int(TOTAL_ITEMS));

	size_t buckets = map->bucket_count();

	map->initialize(true);

	UT_ASSERT(map->bucket_count() == buckets);

	UT_ASSERT(map->size() == TOTAL_ITEMS);

	map->initialize();

	UT_ASSERT(map->bucket_count() == buckets);

	UT_ASSERT(map->size() == TOTAL_ITEMS);

	map->clear();

	UT_ASSERT(map->size() == 0);

	UT_ASSERT(std::distance(map->begin(), map->end()) == 0);
}

/*
 * insert_and_erase_test -- (internal) test insert and erase operations
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
template <typename MapType>
void
insert_and_erase_test(nvobj::pool<root> &pop,
		      nvobj::persistent_ptr<MapType> &map)
{
	const size_t NUMBER_ITEMS_INSERT = 50;

	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 8;

	UT_ASSERT(map != nullptr);

	map->initialize();

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * NUMBER_ITEMS_INSERT;
		int end = begin + int(NUMBER_ITEMS_INSERT) / 2;
		for (int i = begin; i < end; ++i) {
			bool res = map->insert(create_value(pop, *map, i));
			UT_ASSERT(res == true);

			res = map->erase(i);
			UT_ASSERT(res == true);

			UT_ASSERT(map->count(i) == 0);
		}
	});

	UT_ASSERT(map->size() == 0);
}

/*
 * insert_and_erase_test -- (internal) test insert and erase operations
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
template <typename MapType>
void
insert_erase_lookup_test(nvobj::pool<root> &pop,
			 nvobj::persistent_ptr<MapType> &map)
{
	const size_t NUMBER_ITEMS_INSERT = 50;

	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 4;

	UT_ASSERT(map != nullptr);

	map->initialize();

	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				map->insert(create_value(pop, *map, i));
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
				typename MapType::accessor acc;
				bool res = map->find(acc, i);

				if (res) {
					UT_ASSERT(acc->first == i);
					UT_ASSERT(acc->second >= i);
					increase_element(pop, acc->second);
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
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->cons1 = nvobj::make_persistent<
				persistent_map_type_int>();
			pop.root()->cons2 = nvobj::make_persistent<
				persistent_map_type_string>();

			pop.root()->vec = nvobj::make_persistent<
				pmem::obj::experimental::vector<
					persistent_map_type_string::
						value_type>>();
			pop.root()->vec->reserve(MAX_ITEMS);
			for (size_t i = 0; i < MAX_ITEMS; i++)
				pop.root()->vec->emplace_back(
					std::to_string(i), std::to_string(i));
		});
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	insert_and_lookup_test(pop, pop.root()->cons1);
	insert_and_erase_test(pop, pop.root()->cons1);
	insert_erase_lookup_test(pop, pop.root()->cons1);

	insert_and_lookup_test(pop, pop.root()->cons2);
	insert_and_erase_test(pop, pop.root()->cons2);
	insert_erase_lookup_test(pop, pop.root()->cons2);

	pop.close();

	return 0;
}
