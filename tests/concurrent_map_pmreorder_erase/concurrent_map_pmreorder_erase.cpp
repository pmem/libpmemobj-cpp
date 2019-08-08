// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * concurrent_map_pmreorder_erase.cpp --
 * pmem::obj::experimental::concurrent_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <future>
#include <iostream>

#include <libpmemobj++/experimental/concurrent_map.hpp>

#define LAYOUT "persistent_concurrent_map"

namespace nvobj = pmem::obj;

namespace
{

typedef nvobj::experimental::concurrent_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

typedef persistent_map_type::value_type value_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

static constexpr int elements[] = {
	1, 2, 3, 2 + 255, 3 + 255, 4 + 255,
};

static constexpr int len_elements =
	static_cast<int>(sizeof(elements) / sizeof(elements[0]));

/*
 * check_exist -- (internal) check existence of an element
 */
void
check_exist(nvobj::persistent_ptr<persistent_map_type> &map, int element,
	    bool exists)
{
	auto it = map->find(element);
	UT_ASSERTeq(it != map->end(), exists);

	if (exists) {
		UT_ASSERTeq(it->first, element);
		UT_ASSERTeq(it->second, element);
	}
}

/*
 * test_init -- (internal) init test
 * pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
test_init(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;
	persistent_map->runtime_initialize();

	for (int i = 0; i < len_elements; i++)
		persistent_map->insert(value_type(elements[i], elements[i]));

	for (int i = 0; i < len_elements; i++) {
		check_exist(persistent_map, elements[i], true);
		UT_ASSERTeq(persistent_map->count(elements[i]), 1);
	}
}

/*
 * test_erase -- (internal) test
 * pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
test_erase(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;
	persistent_map->runtime_initialize();

	auto size = static_cast<typename persistent_map_type::difference_type>(
		persistent_map->size());

	UT_ASSERTeq(
		std::distance(persistent_map->begin(), persistent_map->end()),
		size);

	int i = 1; /* remove this element */

	UT_ASSERTeq(persistent_map->count(elements[i]), 1);
	check_exist(persistent_map, elements[i], true);

	persistent_map->unsafe_erase(elements[i]);

	UT_ASSERTeq(persistent_map->count(elements[i]), 0);
	check_exist(persistent_map, elements[i], false);

	persistent_map->emplace(elements[i], elements[i]);
	check_exist(persistent_map, elements[i], true);

	persistent_map->unsafe_erase(elements[i]);
	check_exist(persistent_map, elements[i], false);
}

void
check_consistency(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;
	persistent_map->runtime_initialize();

	auto size = static_cast<typename persistent_map_type::difference_type>(
		persistent_map->size());

	UT_ASSERTeq(
		std::distance(persistent_map->begin(), persistent_map->end()),
		size);

	int count = 0;

	for (int i = 0; i < len_elements; i++) {
		if (persistent_map->count(elements[i])) {
			count++;
			check_exist(persistent_map, elements[i], true);
		} else {
			check_exist(persistent_map, elements[i], false);
		}
	}

	UT_ASSERTeq(count, size);
}

} /* namespace */

static void
test(int argc, char *argv[])
{
	if (argc != 3 || strchr("coe", argv[1][0]) == nullptr)
		UT_FATAL("usage: %s <c|o|e> file-name", argv[0]);

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

			pmem::obj::transaction::run(pop, [&] {
				pop.root()->cons = nvobj::make_persistent<
					persistent_map_type>();
			});

			test_init(pop);
		} else if (argv[1][0] == 'e') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			test_erase(pop);
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
