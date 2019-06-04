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
 * concurrent_hash_map_reorder.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <pmemcheck.h>
#include <vector>

#define LIBPMEMOBJ_CPP_CONCURRENT_HASH_MAP_USE_ATOMIC_ALLOCATOR 1

#include <libpmemobj++/experimental/concurrent_hash_map.hpp>

#define LAYOUT "persistent_concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

struct identity {
	static unsigned
	hash(int a)
	{
		return static_cast<unsigned>(a);
	}

	static bool
	equal(int a, int b)
	{
		return a == b;
	}
};

typedef nvobj::experimental::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>,
						 identity>
	persistent_map_type;

typedef persistent_map_type::value_type value_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

static constexpr int mask = 256;

static constexpr int NUMBER_OF_INSERTS = 8;

/*
 * test_insert -- (internal) run several inserts to different buckets
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
test_insert(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;

	for (int i = 0; i < NUMBER_OF_INSERTS / 2; i++)
		persistent_map->insert(value_type(i, i));

	for (int i = 0; i < NUMBER_OF_INSERTS / 2 - 1; i++)
		persistent_map->insert(value_type(i + mask, i + mask));

	{
		typename persistent_map_type::accessor accessor;
		UT_ASSERTeq(persistent_map->find(accessor, 1), true);
	}
	{
		typename persistent_map_type::accessor accessor;
		UT_ASSERTeq(persistent_map->find(accessor, 2), true);
	}
	{
		typename persistent_map_type::accessor accessor;
		UT_ASSERTeq(persistent_map->find(accessor, 1 + mask), true);
	}

	persistent_map->insert(value_type(NUMBER_OF_INSERTS / 2 - 1 + mask,
					  NUMBER_OF_INSERTS / 2 - 1 + mask));
}

void
run_multiple_threads(int concurrency, nvobj::pool<root> &pop)
{
	auto map = pop.root()->cons;

	std::vector<std::thread> threads;
	threads.reserve(static_cast<size_t>(concurrency));

	for (int i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0; i < 10 * concurrency; ++i) {
				map->insert(
					persistent_map_type::value_type(i, i));
			}
		});
	}

	for (int i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0; i < 10 * concurrency; ++i) {
				map->erase(i);
			}
		});
	}

	for (int i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0; i < 10 * concurrency; ++i) {
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
}

void
check_consistency(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;

	persistent_map->initialize();

	auto size = static_cast<typename persistent_map_type::difference_type>(
		persistent_map->size());

	UT_ASSERTeq(
		std::distance(persistent_map->begin(), persistent_map->end()),
		size);

	for (int i = 0; i < size; i++) {
		auto element = i < NUMBER_OF_INSERTS / 2
			? i
			: i + mask - NUMBER_OF_INSERTS / 2;
		std::cout << element << std::endl;
		UT_ASSERTeq(persistent_map->count(element), 1);

		typename persistent_map_type::accessor accessor;
		UT_ASSERTeq(persistent_map->find(accessor, element), true);

		UT_ASSERTeq(accessor->first, element);
		UT_ASSERTeq(accessor->second, element);

		if (i == NUMBER_OF_INSERTS - 1)
			break;
	}

	for (int i = size; i < NUMBER_OF_INSERTS; i++) {
		auto element = i < NUMBER_OF_INSERTS / 2
			? i
			: i + mask - NUMBER_OF_INSERTS / 2;
		UT_ASSERTeq(persistent_map->count(element), 0);
	}

	if (size == NUMBER_OF_INSERTS + 1) {
		typename persistent_map_type::accessor accessor;

		UT_ASSERTeq(persistent_map->count(1000), 1);
		UT_ASSERTeq(persistent_map->find(accessor, 1000), 1);
	}

	run_multiple_threads(4, pop);
}
}

int
main(int argc, char *argv[])
{
	if (argc != 3 || strchr("coi", argv[1][0]) == nullptr)
		UT_FATAL("usage: %s <c|o|i> file-name", argv[0]);

	const char *path = argv[2];

	nvobj::pool<root> pop;

	try {
		if (argv[1][0] == 'o') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			check_consistency(pop);
		} else if (argv[1][0] == 'c') {
			pop = nvobj::pool<root>::create(path, LAYOUT,
							PMEMOBJ_MIN_POOL * 20,
							S_IWUSR | S_IRUSR);

			nvobj::make_persistent_atomic<persistent_map_type>(
				pop, pop.root()->cons);
			pop.root()->cons->insert(value_type(1, 1));
		} else if (argv[1][0] == 'i') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			test_insert(pop);
		}
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	pop.close();

	return 0;
}
