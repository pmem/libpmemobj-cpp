// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * concurrent_hash_map_insert_erase.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "../concurrent_hash_map/concurrent_hash_map_test.hpp"
#include "tests/wrap_pmemobj_defrag.h"
#include "unittest.hpp"

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name [defrag:0|1]", argv[0]);
	}

	const char *path = argv[1];
	int defrag = 0;
	if (argc == 3)
		defrag = atoi(argv[2]);

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

	/* Test that scoped_lock traits is working correctly */
#if LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX
	UT_ASSERT(pmem::obj::concurrent_hash_map_internal::scoped_lock_traits<
			  tbb::spin_rw_mutex::scoped_lock>::
			  initial_rw_state(true) == false);
#else
	UT_ASSERT(pmem::obj::concurrent_hash_map_internal::scoped_lock_traits<
			  pmem::obj::concurrent_hash_map_internal::
				  shared_mutex_scoped_lock<
					  pmem::obj::shared_mutex>>::
			  initial_rw_state(true) == true);
#endif

	size_t concurrency = 8;
	if (On_drd)
		concurrency = 2;
	std::cout << "Running tests for " << concurrency << " threads"
		  << std::endl;

	insert_and_erase_test<persistent_map_type::accessor,
			      persistent_map_type::value_type>(pop,
							       concurrency);

	insert_erase_count_test(pop, concurrency);

	insert_mt_test(pop, concurrency);

	insert_erase_lookup_test(pop, concurrency, defrag);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
