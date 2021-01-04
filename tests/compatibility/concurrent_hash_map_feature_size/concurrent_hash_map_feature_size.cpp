// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/*
 * concurrent_hash_map_feature_size.cpp -- pmem::obj::concurrent_hash_map test
 *		used to check compatibility between different binaries.
 */

#include "../concurrent_hash_map/concurrent_hash_map_traits.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <libpmemobj++/container/concurrent_hash_map.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

typedef nvobj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

static const size_t items_remove = 10;
static const size_t concurrency = 4;
static const size_t thread_insert_num = 50;

void
init(nvobj::pool<root> &pop)
{
	PRINT_TEST_PARAMS;

	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, thread_insert_num * concurrency);

	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * (items_remove + thread_insert_num);
		int end = begin + int(items_remove + thread_insert_num);

		for (int i = begin; i < end; ++i) {
			auto val = persistent_map_type::value_type(i, i);
			test.insert(val);
		}
	});

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * items_remove;
		int end = begin + int(items_remove);

		for (int i = begin; i < end; ++i) {
			test.erase(i);
		}
	});

	test.check_consistency();
}

void
verify(nvobj::pool<root> &pop)
{
	size_t expected_size = concurrency * thread_insert_num;

	ConcurrentHashMapTestPrimitives<root, persistent_map_type> test(
		pop, pop.root()->cons, expected_size);

	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	test.check_items_count(expected_size);
	test.check_consistency();

	for (int i = int(items_remove * concurrency);
	     i < int(items_remove * concurrency + expected_size); ++i) {
		test.check_item<persistent_map_type::const_accessor>(i, i);
	}

	size_t insert_num = 10;

	parallel_exec(concurrency, [&](size_t thread_id) {
		int begin = thread_id * (items_remove + thread_insert_num);
		int end = begin + int(insert_num);

		for (int i = begin; i < end; ++i) {
			auto val = persistent_map_type::value_type(
				int(expected_size +
				    items_remove * concurrency) +
					i,
				i);
			test.insert(val);
		}
	});

	test.check_items_count(expected_size + insert_num * concurrency);

	test.clear();
	test.check_items_count(0);
}

static void
test(int argc, char *argv[])
{
	if (argc < 3) {
		UT_FATAL("usage: %s file-name [c|o]", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	auto create = std::string(argv[2]) == "c";

	if (create) {
		try {
			pop = nvobj::pool<root>::create(path, LAYOUT,
							PMEMOBJ_MIN_POOL * 20,
							S_IWUSR | S_IRUSR);
			pmem::obj::transaction::run(pop, [&] {
				pop.root()->cons = nvobj::make_persistent<
					persistent_map_type>();
			});
		} catch (pmem::pool_error &pe) {
			UT_FATAL("!pool::create: %s %s", pe.what(), path);
		}

		init(pop);
	} else {
		pop = nvobj::pool<root>::open(path, LAYOUT);

		verify(pop);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
