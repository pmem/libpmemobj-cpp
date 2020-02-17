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

template <class C>
void
test_contiguous(const C &c)
{
	for (size_t i = 0; i < c.size(); ++i)
		UT_ASSERT(*(c.begin() + (ptrdiff_t)i) ==
			  *(std::addressof(*c.begin()) + (ptrdiff_t)i));
}

struct Testcase1 {
	typedef double T;
	typedef pmem::obj::array<T, 3> C;
	C c;

	void
	run()
	{
		test_contiguous(C());
		test_contiguous(c);
	}
};

struct root {
	pmem::obj::persistent_ptr<Testcase1> r1;
};

void
run(pmem::obj::pool<root> &pop)
{
	try {
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1 =
				pmem::obj::make_persistent<Testcase1>();
		});

		pmem::obj::transaction::run(pop,
					    [&] { pop.root()->r1->run(); });
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
		pop = pmem::obj::pool<root>::create(path, "contiguous.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
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
