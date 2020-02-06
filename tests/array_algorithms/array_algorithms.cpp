// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "unittest.hpp"

#include <algorithm>
#include <iterator>

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct TestSort {
#ifdef NO_GCC_AGGREGATE_INITIALIZATION_BUG
	pmem::obj::array<double, 10> c = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
#else
	pmem::obj::array<double, 10> c = {{10, 9, 8, 7, 6, 5, 4, 3, 2, 1}};
#endif
	void
	sort_single_element_snapshot()
	{
		std::sort(c.begin(), c.end());

		pmem::obj::array<double, 10> expected = {1, 2, 3, 4, 5,
							 6, 7, 8, 9, 10};

		UT_ASSERT(c == expected);
	}

	void
	sort_range_snapshot()
	{
		auto slice = c.range(0, c.size(), 2);

		std::sort(slice.begin(), slice.end());

		pmem::obj::array<double, 10> expected = {1, 2, 3, 4, 5,
							 6, 7, 8, 9, 10};

		UT_ASSERT(c == expected);
	}
};

struct root {
	pmem::obj::persistent_ptr<TestSort> test_sort;
};

void
test_sort_single_element(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test_sort = pmem::obj::make_persistent<TestSort>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test_sort->sort_single_element_snapshot();

			pmem::obj::transaction::abort(0);
			UT_ASSERT(0);
		});
	} catch (pmem::manual_tx_abort &) {
		pmem::obj::array<double, 10> expected = {10, 9, 8, 7, 6,
							 5,  4, 3, 2, 1};
		UT_ASSERT(r->test_sort->c == expected);
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<TestSort>(r->test_sort);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
test_sort_range(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test_sort = pmem::obj::make_persistent<TestSort>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test_sort->sort_range_snapshot();

			pmem::obj::transaction::abort(0);
			UT_ASSERT(0);
		});
	} catch (pmem::manual_tx_abort &) {
		pmem::obj::array<double, 10> expected = {10, 9, 8, 7, 6,
							 5,  4, 3, 2, 1};
		UT_ASSERT(r->test_sort->c == expected);
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<TestSort>(r->test_sort);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];

	auto pop = pmem::obj::pool<root>::create(
		path, "ArrayTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_sort_single_element(pop);
	test_sort_range(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
