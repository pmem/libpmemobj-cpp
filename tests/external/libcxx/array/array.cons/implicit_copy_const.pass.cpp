//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018-2021, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct NoDefault {
	NoDefault(int)
	{
	}
};

struct Testcase1 {
	typedef double T;
	typedef pmem::obj::array<const T, 3> C;
	C c = {{1.1, 2.2, 3.3}};
	C c2 = c;

	void
	run()
	{
		((void)c2);
		static_assert(std::is_copy_constructible<C>::value, "");
	}
};

struct Testcase2 {
	// const arrays of size 0 should disable the implicit copy
	// assignment operator.
	typedef double T;
	typedef pmem::obj::array<const T, 0> C;
	C c = {{}};
	C c2 = c;

	void
	run()
	{
		((void)c2);
		static_assert(std::is_copy_constructible<C>::value, "");
	}
};

struct Testcase3 {
	typedef NoDefault T;
	typedef pmem::obj::array<const T, 0> C;
	C c = {{}};
	C c2 = c;
	void
	run()
	{
		((void)c2);
		static_assert(std::is_copy_constructible<C>::value, "");
	}
};

struct root {
	pmem::obj::persistent_ptr<Testcase1> r1;
	pmem::obj::persistent_ptr<Testcase2> r2;
	pmem::obj::persistent_ptr<Testcase3> r3;
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
		});

		pop.root()->r1->run();
		pop.root()->r2->run();
		pop.root()->r3->run();
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
		pop = pmem::obj::pool<root>::create(path, "implicit_copy.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
