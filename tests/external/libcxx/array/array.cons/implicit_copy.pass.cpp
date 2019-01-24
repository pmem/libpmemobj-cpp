//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem_exp = pmem::obj::experimental;

struct NoDefault {
	NoDefault(int)
	{
	}
};

struct Testcase1 {
	typedef double T;
	typedef pmem_exp::array<T, 3> C;
	C c = {{1.1, 2.2, 3.3}};
	C c2 = c;
	C c3, c4;

	Testcase1() : c3(c), c4(std::move(c3))
	{
	}

	void
	run()
	{
		c2 = c;
		static_assert(std::is_copy_constructible<C>::value, "");
		static_assert(std::is_copy_assignable<C>::value, "");
	}
};

struct Testcase2 {
	typedef double T;
	typedef pmem_exp::array<const T, 3> C;
	C c = {{1.1, 2.2, 3.3}};
	C c2 = c;

	void
	run()
	{
		((void)c2);
		static_assert(std::is_copy_constructible<C>::value, "");
	}
};

struct Testcase3 {
	typedef double T;
	typedef pmem_exp::array<T, 0> C;
	C c = {{}};
	C c2 = c;
	void
	run()
	{
		c2 = c;
		static_assert(std::is_copy_constructible<C>::value, "");
		static_assert(std::is_copy_assignable<C>::value, "");
	}
};

struct Testcase4 {
	// const arrays of size 0 should disable the implicit copy
	// assignment operator.
	typedef double T;
	typedef pmem_exp::array<const T, 0> C;
	C c = {{}};
	C c2 = c;

	void
	run()
	{
		((void)c2);
		static_assert(std::is_copy_constructible<C>::value, "");
	}
};

struct Testcase5 {
	typedef NoDefault T;
	typedef pmem_exp::array<T, 0> C;
	C c = {{}};
	C c2 = c;

	void
	run()
	{
		c2 = c;
		static_assert(std::is_copy_constructible<C>::value, "");
		static_assert(std::is_copy_assignable<C>::value, "");
	}
};

struct Testcase6 {
	typedef NoDefault T;
	typedef pmem_exp::array<const T, 0> C;
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
	pmem::obj::persistent_ptr<Testcase4> r4;
	pmem::obj::persistent_ptr<Testcase5> r5;
	pmem::obj::persistent_ptr<Testcase6> r6;
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
			pop.root()->r6 =
				pmem::obj::make_persistent<Testcase6>();
		});

		pop.root()->r1->run();
		pop.root()->r2->run();
		pop.root()->r3->run();
		pop.root()->r4->run();
		pop.root()->r5->run();
		pop.root()->r6->run();
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
		pop = pmem::obj::pool<root>::create(path, "implicit_copy.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	return 0;
}
