// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * concurrent_hash_map_rehash_deadlock.cpp -- pmem::obj::concurrent_hash_map
 * test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <vector>

#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/container/string.hpp>

#include "tests/wrap_pmemobj_defrag.h"

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

typedef nvobj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

/*
 * recursive_rehashing_deadlock_test -- (internal) test recursive rehashing in
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
recursive_rehashing_deadlock_test(nvobj::pool<root> &pop)
{

	PRINT_TEST_PARAMS;

	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	/*
	 * Insert many elements to the hash map in a way
	 * that 128 buckets in 5 following segments are not rehashed:
	 * - buckets #128-#255 contain numbers: 3968-4095
	 * - buckets #384-#511 are empty
	 * - buckets #896-#1023 are empty
	 * - buckets #1920-#2047 are empty
	 * - buckets #3968-#4095 are empty
	 *
	 * A reference (find()) to the buckets #3968-#4095 will cause
	 * recursive rehashing of the previous buckets.
	 *
	 * For example 'find(acc, 4095)' will cause taking locks
	 * on the following 5 buckets and recursive rehashing of them:
	 * 4095, 2047, 1023, 511 and 255.
	 */
	int skip = 0;
	for (int i = 4095; i >= 2048; i--) {
		int hb = i & 255;
		if (hb == 128) {
			skip = 1;
		}
		if (skip && hb >= 128) {
			continue;
		}
		map->insert(persistent_map_type::value_type(i, i));
	}

	for (int i = 4095; i >= 4090; i--) {
		persistent_map_type::accessor acc;
		map->find(acc, i);
		map->defragment();
	}
}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->cons =
				nvobj::make_persistent<persistent_map_type>();
		});
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	recursive_rehashing_deadlock_test(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
