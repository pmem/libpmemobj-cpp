// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * concurrent_map_insert_reopen.cpp -- pmem::obj::experimental::concurrent_map
 * test
 *
 */

#include "unittest.hpp"
#include <libpmemobj++/experimental/concurrent_map.hpp>

template <typename MapType>
void
check_size(MapType *map, size_t expected_size)
{
	UT_ASSERT(map->size() == expected_size);
	UT_ASSERT(std::distance(map->begin(), map->end()) ==
		  int(expected_size));
}

namespace nvobj = pmem::obj;

typedef nvobj::experimental::concurrent_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

static const std::string LAYOUT = "concurrent_map";

/*
 * insert_reopen_test -- (internal) test insert operations and verify
 * consistency after reopen
 * pmem::obj::experimental::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_reopen_test(nvobj::pool<root> &pop, std::string path,
		   size_t concurrency = 4)
{

	using value_type = persistent_map_type::value_type;
	PRINT_TEST_PARAMS;

	size_t thread_items = 50;
	const size_t expected_size = thread_items * concurrency;

	{
		auto map = pop.root()->cons;

		UT_ASSERT(map != nullptr);

		map->runtime_initialize();

		parallel_exec(concurrency, [&](size_t thread_id) {
			int begin = thread_id * thread_items;
			int end = begin + int(thread_items);
			for (int i = begin; i < end; ++i) {
				map->insert(value_type(i, i));
			}
		});

		check_size(map.get(), expected_size);

		map->insert(value_type(expected_size + 1, 1));
		map->unsafe_erase(expected_size + 1);

		pop.close();
	}

	{
		pop = nvobj::pool<root>::open(path, LAYOUT);

		auto map = pop.root()->cons;

		UT_ASSERT(map != nullptr);

		map->runtime_initialize();

		check_size(map.get(), expected_size);

		parallel_exec(concurrency, [&](size_t thread_id) {
			int begin = thread_id * thread_items;
			int end = begin + int(thread_items);
			for (int i = begin; i < end; ++i) {
				map->insert(
					value_type(i + int(expected_size), i));
			}
		});

		check_size(map.get(), expected_size * 2);
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
