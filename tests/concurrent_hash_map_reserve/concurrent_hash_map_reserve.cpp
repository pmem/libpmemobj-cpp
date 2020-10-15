// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../concurrent_hash_map/concurrent_hash_map_string_test.hpp"
#include "unittest.hpp"

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

/*
 * reserve -- basic reserve test of
 * pmem::obj::concurrent_hash_map<pmem::obj::string, pmem::obj::string>
 */
void
reserve_test(nvobj::pool<root> &pop)
{
	const size_t RESERVE_COUNT = 5000;
	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	UT_ASSERT(map->bucket_count() < RESERVE_COUNT);
	UT_ASSERTeq(map->size(), 0);
	map->reserve(RESERVE_COUNT);
	UT_ASSERTeq(map->size(), 0);
	UT_ASSERT(map->bucket_count() >= RESERVE_COUNT);

	map->clear();
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

	reserve_test(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
