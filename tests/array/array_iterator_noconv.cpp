// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <iterator>

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct Testcase1 {
	typedef pmem::obj::array<int, 5> C;
	C c{{0, 1, 2, 3, 4}};

	void
	iterator_index()
	{
		C::iterator i;
		i = c.begin();

		long l = 0;
		long long ll = 0;
		unsigned u = 0;
		unsigned long long ull = 0;

		UT_ASSERT(i[l] == 0);
		UT_ASSERT(i[ll] == 0);
		UT_ASSERT(i[u] == 0);
		UT_ASSERT(i[ull] == 0);

		C::const_iterator j;
		j = c.cbegin();

		UT_ASSERT(j[l] == 0);
		UT_ASSERT(j[ll] == 0);
		UT_ASSERT(j[u] == 0);
		UT_ASSERT(j[ull] == 0);

		UT_ASSERT(i == j);

		C::range_snapshotting_iterator k;
		k = c.range(0, 2, 1).begin();

		UT_ASSERT(k[l] == 0);
		UT_ASSERT(k[ll] == 0);
		UT_ASSERT(k[u] == 0);
		UT_ASSERT(k[ull] == 0);

		UT_ASSERT(i == k);
	}
};

struct root {
	pmem::obj::persistent_ptr<Testcase1> test1;
};

void
run_test1(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test1 = pmem::obj::make_persistent<Testcase1>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(
			pop, [&] { r->test1->iterator_index(); });
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<Testcase1>(r->test1);
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

	run_test1(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
