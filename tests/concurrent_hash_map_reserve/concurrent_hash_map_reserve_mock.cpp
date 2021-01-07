// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../common/mock_tx_alloc.h"
#include "../concurrent_hash_map/concurrent_hash_map_string_test.hpp"
#include "unittest.hpp"

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

/*
 * reserve_insert -- reserve and insert with mock pmemobj_tx_[x]alloc
 * pmem::obj::concurrent_hash_map<pmem::obj::string, pmem::obj::string>
 */
void
reserve_insert(nvobj::pool<root> &pop)
{
	const size_t RESERVE_COUNT = 1000000;

	auto map = pop.root()->cons;
	UT_ASSERT(map != nullptr);

	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, 0);

	UT_ASSERT(map->bucket_count() < RESERVE_COUNT);
	UT_ASSERTeq(map->size(), 0);

	/* insert and check allocations' count */
	test_alloc_counter = 0;
	for (size_t i = 0; i < RESERVE_COUNT; i++) {
		bool ret = test.insert_or_assign(std::to_string(i), i);
		UT_ASSERT(ret);
	}
	auto test_alloc_counter_first = test_alloc_counter;
	UT_ASSERTeq(map->size(), RESERVE_COUNT);

	/* cleanup */
	test.clear();
	UT_ASSERT(map->bucket_count() < RESERVE_COUNT);

	/* insert again (with reserve) and check new allocations' count */
	test_alloc_counter = 0;
	map->reserve(RESERVE_COUNT);
	UT_ASSERTeq(map->size(), 0);
	UT_ASSERT(map->bucket_count() >= RESERVE_COUNT);

	for (size_t i = 0; i < RESERVE_COUNT; i++) {
		bool ret = test.insert_or_assign(std::to_string(i), i);
		UT_ASSERT(ret);
	}
	UT_ASSERT(test_alloc_counter <= test_alloc_counter_first);
	UT_ASSERTeq(map->size(), RESERVE_COUNT);

	test.clear();
}

static void
test(int argc, char *argv[])
{
	if (argc < 1) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];
	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(path, LAYOUT,
						200 * PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->cons =
				nvobj::make_persistent<persistent_map_type>();
		});
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	reserve_insert(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
