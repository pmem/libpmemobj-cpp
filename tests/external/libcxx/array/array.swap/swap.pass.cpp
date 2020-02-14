//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using pmem::obj::swap;

struct NonSwappable {
	NonSwappable()
	{
	}

private:
	NonSwappable(NonSwappable const &);
	NonSwappable &operator=(NonSwappable const &);
};

struct Testcase1 {
	typedef double T;
	typedef pmem::obj::array<T, 3> C;
	C c1 = {{1, 2, 3.5}};
	C c2 = {{4, 5, 6.5}};

	void
	run()
	{
		c1.swap(c2);
		UT_ASSERT(c1.size() == 3);
		UT_ASSERT(c1[0] == 4);
		UT_ASSERT(c1[1] == 5);
		UT_ASSERT(c1[2] == 6.5);
		UT_ASSERT(c2.size() == 3);
		UT_ASSERT(c2[0] == 1);
		UT_ASSERT(c2[1] == 2);
		UT_ASSERT(c2[2] == 3.5);
	}
};

struct Testcase2 {
	typedef double T;
	typedef pmem::obj::array<T, 3> C;
	C c1 = {{1, 2, 3.5}};
	C c2 = {{4, 5, 6.5}};

	void
	run()
	{
		swap(c1, c2);
		UT_ASSERT(c1.size() == 3);
		UT_ASSERT(c1[0] == 4);
		UT_ASSERT(c1[1] == 5);
		UT_ASSERT(c1[2] == 6.5);
		UT_ASSERT(c2.size() == 3);
		UT_ASSERT(c2[0] == 1);
		UT_ASSERT(c2[1] == 2);
		UT_ASSERT(c2[2] == 3.5);
	}
};

struct Testcase3 {
	typedef double T;
	typedef pmem::obj::array<T, 0> C;
	C c1 = {{}};
	C c2 = {{}};
	void
	run()
	{
		c1.swap(c2);
		UT_ASSERT(c1.size() == 0);
		UT_ASSERT(c2.size() == 0);
	}
};

struct Testcase4 {
	typedef double T;
	typedef pmem::obj::array<T, 0> C;
	C c1 = {{}};
	C c2 = {{}};

	void
	run()
	{
		swap(c1, c2);
		UT_ASSERT(c1.size() == 0);
		UT_ASSERT(c2.size() == 0);
	}
};

struct Testcase5 {
	typedef NonSwappable T;
	typedef pmem::obj::array<T, 0> C0;
	C0 l = {{}};
	C0 r = {{}};

	void
	run()
	{
		l.swap(r);
	}
};

struct root {
	pmem::obj::persistent_ptr<Testcase1> r1;
	pmem::obj::persistent_ptr<Testcase2> r2;
	pmem::obj::persistent_ptr<Testcase3> r3;
	pmem::obj::persistent_ptr<Testcase4> r4;
	pmem::obj::persistent_ptr<Testcase5> r5;
};

void
run(pmem::obj::pool<root> &pop)
{
	try {
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1 =
				pmem::obj::make_persistent<Testcase1>();
			pop.root()->r2 =
				pmem::obj::make_persistent<Testcase2>();
			pop.root()->r3 =
				pmem::obj::make_persistent<Testcase3>();
			pop.root()->r4 =
				pmem::obj::make_persistent<Testcase4>();
			pop.root()->r5 =
				pmem::obj::make_persistent<Testcase5>();
		});

		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1->run();
			pop.root()->r2->run();
			pop.root()->r3->run();
			pop.root()->r4->run();
			pop.root()->r5->run();
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(
			path, "swap.pass", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
