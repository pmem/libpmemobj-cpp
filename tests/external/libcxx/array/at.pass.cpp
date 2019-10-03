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
		{
			C::reference r1 = c.at(0);
			UT_ASSERT(r1 == 1);
			r1 = 5.5;
			UT_ASSERT(c.front() == 5.5);
			UT_ASSERT(c.cfront() == 5.5);

			C::reference r2 = c.at(2);
			UT_ASSERT(r2 == 3.5);
			r2 = 7.5;
			UT_ASSERT(c.back() == 7.5);
			UT_ASSERT(c.cback() == 7.5);

			try {
				c.at(3);
				UT_ASSERT(false);
			} catch (const std::out_of_range &) {
			}
		}
		{
			C::const_reference r1 = c.const_at(0);
			UT_ASSERT(r1 == 5.5);

			C::const_reference r2 = c.const_at(2);
			UT_ASSERT(r2 == 7.5);

			try {
				c.const_at(3);
				UT_ASSERT(false);
			} catch (const std::out_of_range &) {
			}
		}
	}
};

struct Testcase2 {
	typedef double T;
	typedef pmem::obj::array<T, 0> C;
	C c = {{}};

	void
	run()
	{
		{
			C const &cc = c;
			try {
				c.at(0);
				UT_ASSERT(false);
			} catch (const std::out_of_range &) {
			}
			try {
				cc.at(0);
				UT_ASSERT(false);
			} catch (const std::out_of_range &) {
			}
		}
		{
			C const &cc = c;
			try {
				c.const_at(0);
				UT_ASSERT(false);
			} catch (const std::out_of_range &) {
			}
			try {
				cc.const_at(0);
				UT_ASSERT(false);
			} catch (const std::out_of_range &) {
			}
		}
	}
};

struct Testcase3 {
	typedef double T;
	typedef pmem::obj::array<T, 3> C;
	const C c = {{1, 2, 3.5}};

	void
	run()
	{
		{
			C::const_reference r1 = c.at(0);
			UT_ASSERT(r1 == 1);

			C::const_reference r2 = c.at(2);
			UT_ASSERT(r2 == 3.5);

			try {
				c.at(3);
				UT_ASSERT(false);
			} catch (const std::out_of_range &) {
			}
		}
		{
			C::const_reference r1 = c.const_at(0);
			UT_ASSERT(r1 == 1);

			C::const_reference r2 = c.const_at(2);
			UT_ASSERT(r2 == 3.5);

			try {
				c.const_at(3);
				UT_ASSERT(false);
			} catch (const std::out_of_range &) {
			}
		}
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

		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1->run();
			pop.root()->r2->run();
			pop.root()->r3->run();
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
			path, "at.pass", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	return 0;
}
