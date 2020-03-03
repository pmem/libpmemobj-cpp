// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * concurrent_hash_map_reorder.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <future>
#include <iostream>

#include <libpmemobj++/container/concurrent_hash_map.hpp>

#define LAYOUT "persistent_concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

typedef nvobj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

typedef persistent_map_type::value_type value_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

/*
 * All elements will go to the same bucket.
 * After rehash, element 0, 1, 4 will remain in the original bucket, rest
 * will go to the new one.
 */
static constexpr int elements[] = {1, 257, 1281, 513, 1025, 769};
static constexpr int elements_size = sizeof(elements) / sizeof(elements[0]);

static constexpr int test_element = 1793;

static constexpr int hash_map_size = 255;

/*
 * Insert test elements.
 */
void
insert(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;

	persistent_map->runtime_initialize();

	auto TEST_KEY = 10000;

	/* Prepare hash_map, so that adding elements[] will cause rehash */
	for (int i = 0; i < static_cast<int>(hash_map_size - elements_size);
	     i++) {
		auto ret = persistent_map->insert(
			value_type(TEST_KEY + i, TEST_KEY + i));
		UT_ASSERT(ret == true);
	}

	for (int i = 0; i < elements_size; i++) {
		auto ret = persistent_map->insert(
			value_type(elements[i], elements[i]));
		UT_ASSERT(ret == true);
	}
}

void
rehash(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;

	persistent_map->runtime_initialize();

	/* Force rehash */
	auto ret = persistent_map->count(test_element);

	/* there is no element test_element */
	UT_ASSERT(ret == false);
}

void
check_consistency(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;

	persistent_map->runtime_initialize();

	auto ret = persistent_map->count(test_element);
	/* there is no element test_element */
	UT_ASSERT(ret == false);

	auto size = static_cast<typename persistent_map_type::difference_type>(
		persistent_map->size());

	UT_ASSERTeq(
		std::distance(persistent_map->begin(), persistent_map->end()),
		hash_map_size);

	UT_ASSERTeq(size, hash_map_size);

	for (int i = 0; i < elements_size; i++) {
		UT_ASSERTeq(persistent_map->count(elements[i]), 1);

		typename persistent_map_type::accessor accessor;
		UT_ASSERTeq(persistent_map->find(accessor, elements[i]), true);

		UT_ASSERTeq(accessor->first, elements[i]);
		UT_ASSERTeq(accessor->second, elements[i]);
	}
}
}

static void
test(int argc, char *argv[])
{
	if (argc != 3 || strchr("cbo", argv[1][0]) == nullptr)
		UT_FATAL("usage: %s <c|b|o> file-name", argv[0]);

	const char *path = argv[2];

	nvobj::pool<root> pop;

	try {
		if (argv[1][0] == 'c') {
			pop = nvobj::pool<root>::create(path, LAYOUT,
							PMEMOBJ_MIN_POOL * 20,
							S_IWUSR | S_IRUSR);

			pmem::obj::transaction::run(pop, [&] {
				pop.root()->cons = nvobj::make_persistent<
					persistent_map_type>();
			});

			insert(pop);
		} else if (argv[1][0] == 'b') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			rehash(pop);
		} else if (argv[1][0] == 'o') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			check_consistency(pop);
		}
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
