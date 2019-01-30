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

struct Testcase1 {
	typedef double T;
	typedef pmem_exp::array<T, 3> C;
	C c = {{1, 2, 3.5}};

	void
	run()
	{
		C::reference r1 = c.front();
		UT_ASSERT(r1 == 1);
		r1 = 5.5;
		UT_ASSERT(c[0] == 5.5);

		C::reference r2 = c.back();
		UT_ASSERT(r2 == 3.5);
		r2 = 7.5;
		UT_ASSERT(c[2] == 7.5);
	}
};

struct Testcase2 {
	typedef double T;
	typedef pmem_exp::array<T, 3> C;
	const C c = {{1, 2, 3.5}};

	void
	run()
	{
		{
			C::const_reference r1 = c.front();
			UT_ASSERT(r1 == 1);

			C::const_reference r2 = c.back();
			UT_ASSERT(r2 == 3.5);
		}
		{
			C::const_reference r1 = c.cfront();
			UT_ASSERT(r1 == 1);

			C::const_reference r2 = c.cback();
			UT_ASSERT(r2 == 3.5);
		}
	}
};

struct Testcase3 {
	typedef double T;
	typedef pmem_exp::array<T, 0> C;
	C c = {{}};

	void
	run()
	{
		C const &cc = c;
		static_assert((std::is_same<decltype(c.front()), T &>::value),
			      "");
		static_assert(
			(std::is_same<decltype(cc.front()), const T &>::value),
			"");
		static_assert((std::is_same<decltype(c.back()), T &>::value),
			      "");
		static_assert(
			(std::is_same<decltype(cc.back()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(c.cfront()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(cc.cfront()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(c.cback()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(cc.cback()), const T &>::value),
			"");
		if (c.size() > (0)) { // always false
			c.front();
			c.back();
			c.cfront();
			c.cback();
			cc.front();
			cc.back();
			cc.cfront();
			cc.cback();
		}
	}
};

struct Testcase4 {
	typedef double T;
	typedef pmem_exp::array<const T, 0> C;
	C c = {{}};

	void
	run()
	{
		C const &cc = c;
		static_assert(
			(std::is_same<decltype(c.front()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(cc.front()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(c.back()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(cc.back()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(c.cfront()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(cc.cfront()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(c.cback()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(cc.cback()), const T &>::value),
			"");
		if (c.size() > (0)) {
			c.front();
			c.back();
			c.cfront();
			c.cback();
			cc.front();
			cc.back();
			cc.cfront();
			cc.cback();
		}
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
		pop = pmem::obj::pool<root>::create(path, "front_back.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	return 0;
}
