// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * array_slice_pmreorder.cpp -- pmem::obj::array test under
 * pmreorder
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <iostream>

#define LAYOUT "pmreorder"

namespace nvobj = pmem::obj;

struct Data {
	void
	increase_elements()
	{
		auto slice = array.range(0, array.size());

		for (auto &e : slice) {
			e++;
		}
	}

	nvobj::array<int, 5> array = {{1, 2, 3, 4, 5}};
};

struct root {
	nvobj::persistent_ptr<Data> ptr;
};

void
init(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->ptr = nvobj::make_persistent<Data>(); });
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_consistent(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop,
					[&] { r->ptr->increase_elements(); });
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
run_inconsistent(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	r->ptr->increase_elements();
	r->ptr.persist();
}

void
check_consistency(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	if (r->ptr->array[0] == 1) {
		auto i = 1;
		for (auto e : r->ptr->array)
			UT_ASSERT(e == i++);
	} else {
		auto i = 2;
		for (auto e : r->ptr->array)
			UT_ASSERT(e == i++);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 3 || strchr("coxi", argv[1][0]) == nullptr)
		UT_FATAL("usage: %s <c|o|x|i> file-name", argv[0]);

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

			init(pop);
		} else if (argv[1][0] == 'i') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			run_inconsistent(pop);
		} else if (argv[1][0] == 'x') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			run_consistent(pop);
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
