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
 * concurrent_map.cpp -- pmem::obj::concurrent_map test
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

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/experimental/concurrent_map.hpp>

#define LAYOUT "concurrent_map"

namespace nvobj = pmem::obj;

namespace
{

struct hetero_less {
	using is_transparent = void;
	template <typename T1, typename T2>
	bool
	operator()(const T1 &lhs, const T2 &rhs) const
	{
		return lhs < rhs;
	}
};

typedef nvobj::experimental::concurrent_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type_int;

typedef nvobj::experimental::concurrent_map<nvobj::string, nvobj::string,
					    hetero_less>
	persistent_map_type_string;

struct root {
	nvobj::persistent_ptr<persistent_map_type_int> cons1;
	nvobj::persistent_ptr<persistent_map_type_string> cons2;
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

std::string
gen_key(persistent_map_type_string &, int i)
{
	return std::to_string(i);
}

int
gen_key(persistent_map_type_int &, int i)
{
	return i;
}

/*
 * emplace_and_lookup_test -- (internal) test emplace and lookup operations
 */
template <typename MapType>
void
emplace_and_lookup_test(nvobj::pool<root> &pop,
			nvobj::persistent_ptr<MapType> &map)
{
	const size_t NUMBER_ITEMS_INSERT = 50;

	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 8;

	size_t TOTAL_ITEMS = NUMBER_ITEMS_INSERT * concurrency;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * NUMBER_ITEMS_INSERT;
		int end = begin + int(NUMBER_ITEMS_INSERT);
		for (int i = begin; i < end; ++i) {
			auto ret = map->emplace(gen_key(*map, i),
						gen_key(*map, i));
			UT_ASSERT(ret.second == true);

			UT_ASSERT(map->count(gen_key(*map, i)) == 1);

			typename MapType::iterator it =
				map->find(gen_key(*map, i));
			UT_ASSERT(it != map->end());
			UT_ASSERT(it->first == gen_key(*map, i));
			UT_ASSERT(it->second == gen_key(*map, i));
		}

		for (int i = begin; i < end; ++i) {
			typename MapType::const_iterator it =
				map->find(gen_key(*map, i));
			UT_ASSERT(it != map->end());
			UT_ASSERT(it->first == gen_key(*map, i));
			UT_ASSERT(it->second == gen_key(*map, i));
		}
	});

	UT_ASSERT(map->size() == TOTAL_ITEMS);

	UT_ASSERT(std::distance(map->begin(), map->end()) == int(TOTAL_ITEMS));

	UT_ASSERT(std::is_sorted(map->begin(), map->end()));

	map->runtime_initialize(true);

	UT_ASSERT(map->size() == TOTAL_ITEMS);

	map->runtime_initialize();

	UT_ASSERT(map->size() == TOTAL_ITEMS);

	map->clear();

	UT_ASSERT(map->size() == 0);

	UT_ASSERT(std::distance(map->begin(), map->end()) == 0);
}
/*
 * emplace_and_lookup_duplicates_test -- (internal) test emplace and lookup
 * operations with duplicates
 */
template <typename MapType>
void
emplace_and_lookup_duplicates_test(nvobj::pool<root> &pop,
				   nvobj::persistent_ptr<MapType> &map)
{
	const size_t NUMBER_ITEMS_INSERT = 50;

	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 4;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	std::vector<std::thread> threads;
	threads.reserve(concurrency * 2);

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				map->emplace(gen_key(*map, i),
					     gen_key(*map, i));
			}
		});
	}

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				auto it = map->find(gen_key(*map, i));

				if (it != map->end()) {
					UT_ASSERT(it->first ==
						  gen_key(*map, i));
					UT_ASSERT(it->second ==
						  gen_key(*map, i));
				}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	for (auto &e : *map) {
		UT_ASSERT(e.first == e.second);
	}

	UT_ASSERT(map->size() == NUMBER_ITEMS_INSERT);

	UT_ASSERT(std::distance(map->begin(), map->end()) ==
		  static_cast<int>(NUMBER_ITEMS_INSERT));

	UT_ASSERT(std::is_sorted(map->cbegin(), map->cend()));
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
		nvobj::transaction::run(pop, [&] {
			pop.root()->cons1 = nvobj::make_persistent<
				persistent_map_type_int>();
			pop.root()->cons2 = nvobj::make_persistent<
				persistent_map_type_string>();
		});
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	emplace_and_lookup_test(pop, pop.root()->cons1);
	emplace_and_lookup_duplicates_test(pop, pop.root()->cons1);

	emplace_and_lookup_test(pop, pop.root()->cons2);
	emplace_and_lookup_duplicates_test(pop, pop.root()->cons2);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<persistent_map_type_int>(
			pop.root()->cons1);
		nvobj::delete_persistent<persistent_map_type_string>(
			pop.root()->cons2);
	});

	pop.close();

	return 0;
}
