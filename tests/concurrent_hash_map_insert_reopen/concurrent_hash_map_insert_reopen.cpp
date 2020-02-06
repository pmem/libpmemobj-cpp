// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * concurrent_hash_map_insert_reopen.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "../concurrent_hash_map/concurrent_hash_map_test.hpp"
#include "unittest.hpp"

/* When this is defined we test deprecated runtime_initialize() method which
 * is needed for compatibility. We test new runtime_initialize() otherwise. */
#ifdef USE_DEPRECATED_RUNTIME_INITIALIZE
#define RUNTIME_INITIALIZE runtime_initialize(true)
#else
#define RUNTIME_INITIALIZE runtime_initialize()
#endif

/*
 * insert_reopen_test -- (internal) test insert operations and verify
 * consistency after reopen
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_reopen_test(nvobj::pool<root> &pop, std::string path,
		   size_t concurrency = 4)
{
	PRINT_TEST_PARAMS;

	size_t thread_items = 50;

	{
		ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
			pop, pop.root()->cons, thread_items * concurrency);

		auto map = pop.root()->cons;

		UT_ASSERT(map != nullptr);

		map->runtime_initialize();

		parallel_exec(concurrency, [&](size_t thread_id) {
			int begin = thread_id * thread_items;
			int end = begin + int(thread_items);
			for (int i = begin; i < end; ++i) {
				persistent_map_type::value_type val(i, i);
				test.insert<persistent_map_type::accessor>(val);
			}
		});

		test.check_items_count();

		pop.close();
	}

	{
		size_t already_inserted_num = concurrency * thread_items;

		pop = nvobj::pool<root>::open(path, LAYOUT);

		ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
			pop, pop.root()->cons, thread_items * concurrency);

		auto map = pop.root()->cons;

		UT_ASSERT(map != nullptr);

		map->RUNTIME_INITIALIZE;

		test.check_items_count();

		parallel_exec(concurrency, [&](size_t thread_id) {
			int begin = thread_id * thread_items;
			int end = begin + int(thread_items);
			for (int i = begin; i < end; ++i) {
				persistent_map_type::value_type val(
					i + int(already_inserted_num), i);
				test.insert<persistent_map_type::accessor>(val);
			}
		});

		test.check_items_count(already_inserted_num * 2);
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

	size_t concurrency = 8;
	if (On_drd)
		concurrency = 2;
	std::cout << "Running tests for " << concurrency << " threads"
		  << std::endl;

	insert_reopen_test(pop, path, concurrency);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
