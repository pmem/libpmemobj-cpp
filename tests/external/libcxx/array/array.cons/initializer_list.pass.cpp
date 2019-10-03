//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018-2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct Testcase1 {
	typedef double T;
	typedef pmem::obj::array<T, 3> C;
	C c = {{1, 2, 3.5}};

	void
	run()
	{
		UT_ASSERT(c.size() == 3);
		UT_ASSERT(c[0] == 1);
		UT_ASSERT(c[1] == 2);
		UT_ASSERT(c[2] == 3.5);
	}
};

struct Testcase2 {
	typedef double T;
	typedef pmem::obj::array<T, 0> C;
	C c = {{}};

	void
	run()
	{
		UT_ASSERT(c.size() == 0);
	}
};

struct Testcase3 {
	typedef double T;
	typedef pmem::obj::array<T, 3> C;
	C c = {{1}};

	void
	run()
	{
		UT_ASSERT(c.size() == 3.0);
		UT_ASSERT(c[0] == 1);
	}
};

struct Testcase4 {
	typedef int T;
	typedef pmem::obj::array<T, 1> C;
	C c = {{}};

	void
	run()
	{
		UT_ASSERT(c.size() == 1);
	}
};

struct root {
	pmem::obj::persistent_ptr<Testcase1> r1;
	pmem::obj::persistent_ptr<Testcase2> r2;
	pmem::obj::persistent_ptr<Testcase3> r3;
	pmem::obj::persistent_ptr<Testcase4> r4;
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
		});

		/* XXX: operator[] needs transaction */
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1->run();
			pop.root()->r2->run();
			pop.root()->r3->run();
			pop.root()->r4->run();
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(
			path, "initializer_list.pass", PMEMOBJ_MIN_POOL,
			S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	return 0;
}
