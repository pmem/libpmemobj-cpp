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
#include <cstddef>

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct Testcase1 {
	typedef double T;
	typedef pmem::obj::array<T, 3> C;
	const C c = {{1, 2, 3.5}};
	C cc = {{1, 2, 3.5}};

	void
	run()
	{
		{
			const T *p = c.data();
			UT_ASSERT(p[0] == 1);
			UT_ASSERT(p[1] == 2);
			UT_ASSERT(p[2] == 3.5);
		}
		{
			const T *p = cc.cdata();
			UT_ASSERT(p[0] == 1);
			UT_ASSERT(p[1] == 2);
			UT_ASSERT(p[2] == 3.5);
		}
	}
};

struct Testcase2 {
	typedef std::max_align_t T;
	typedef pmem::obj::array<T, 0> C;
	const C c = {{}};
	C cc = {{}};

	void
	run()
	{
		{
			const T *p = cc.cdata();
			std::uintptr_t pint =
				reinterpret_cast<std::uintptr_t>(p);
			UT_ASSERT(pint % alignof(std::max_align_t) == 0);
		}
		{
			const T *p = cc.cdata();
			std::uintptr_t pint =
				reinterpret_cast<std::uintptr_t>(p);
			UT_ASSERT(pint % alignof(std::max_align_t) == 0);
		}
	}
};

struct root {
	pmem::obj::persistent_ptr<Testcase1> r1;
	pmem::obj::persistent_ptr<Testcase2> r2;
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
		});

		pop.root()->r1->run();
		pop.root()->r2->run();
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
		pop = pmem::obj::pool<root>::create(path, "data_const.pass",
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
