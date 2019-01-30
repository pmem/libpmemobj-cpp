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
	typedef pmem_exp::array<int, 5> C;
	C c;

	void
	run()
	{
		C::iterator i;
		i = c.begin();
		C::const_iterator j;
		j = c.cbegin();
		UT_ASSERT(i == j);
		C::const_iterator k;
		k = static_cast<const C &>(c).begin();
		UT_ASSERT(i == k);
	}
};

struct Testcase2 {
	typedef pmem_exp::array<int, 0> C;
	C c;

	void
	run()
	{
		C::iterator i;
		i = c.begin();
		C::const_iterator j;
		j = c.cbegin();
		UT_ASSERT(i == j);
		C::const_iterator k;
		k = static_cast<const C &>(c).begin();
		UT_ASSERT(i == k);
	}
};

struct Testcase3 {
	typedef pmem_exp::array<int, 5> C;
	C c;

	void
	run()
	{
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);
		UT_ASSERT(ii1 == cii);

		UT_ASSERT(!(ii1 != ii2));
		UT_ASSERT(!(ii1 != cii));

		UT_ASSERT(c.begin() == pmem_exp::begin(c));
		UT_ASSERT(c.cbegin() == pmem_exp::cbegin(c));
		UT_ASSERT(c.rbegin() == pmem_exp::rbegin(c));
		UT_ASSERT(c.crbegin() == pmem_exp::crbegin(c));
		UT_ASSERT(c.end() == pmem_exp::end(c));
		UT_ASSERT(c.cend() == pmem_exp::cend(c));
		UT_ASSERT(c.rend() == pmem_exp::rend(c));
		UT_ASSERT(c.crend() == pmem_exp::crend(c));

		UT_ASSERT(pmem_exp::begin(c) != pmem_exp::end(c));
		UT_ASSERT(pmem_exp::rbegin(c) != pmem_exp::rend(c));
		UT_ASSERT(pmem_exp::cbegin(c) != pmem_exp::cend(c));
		UT_ASSERT(pmem_exp::crbegin(c) != pmem_exp::crend(c));
	}
};

struct Testcase4 {
	typedef pmem_exp::array<int, 0> C;
	C c;

	void
	run()
	{
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);

		UT_ASSERT(!(ii1 != ii2));

		UT_ASSERT((ii1 == cii));
		UT_ASSERT((cii == ii1));
		UT_ASSERT(!(ii1 != cii));
		UT_ASSERT(!(cii != ii1));
		UT_ASSERT(!(ii1 < cii));
		UT_ASSERT(!(cii < ii1));
		UT_ASSERT((ii1 <= cii));
		UT_ASSERT((cii <= ii1));
		UT_ASSERT(!(ii1 > cii));
		UT_ASSERT(!(cii > ii1));
		UT_ASSERT((ii1 >= cii));
		UT_ASSERT((cii >= ii1));
		UT_ASSERT(cii - ii1 == 0);
		UT_ASSERT(ii1 - cii == 0);

		UT_ASSERT(c.begin() == pmem_exp::begin(c));
		UT_ASSERT(c.cbegin() == pmem_exp::cbegin(c));
		UT_ASSERT(c.rbegin() == pmem_exp::rbegin(c));
		UT_ASSERT(c.crbegin() == pmem_exp::crbegin(c));
		UT_ASSERT(c.end() == pmem_exp::end(c));
		UT_ASSERT(c.cend() == pmem_exp::cend(c));
		UT_ASSERT(c.rend() == pmem_exp::rend(c));
		UT_ASSERT(c.crend() == pmem_exp::crend(c));

		UT_ASSERT(pmem_exp::begin(c) == pmem_exp::end(c));
		UT_ASSERT(pmem_exp::rbegin(c) == pmem_exp::rend(c));
		UT_ASSERT(pmem_exp::cbegin(c) == pmem_exp::cend(c));
		UT_ASSERT(pmem_exp::crbegin(c) == pmem_exp::crend(c));
	}
};

struct Testcase5 {
	typedef pmem_exp::array<int, 5> C;
	C c{{0, 1, 2, 3, 4}};

	void
	run()
	{
		UT_ASSERT(c.begin() == pmem_exp::begin(c));
		UT_ASSERT(c.cbegin() == pmem_exp::cbegin(c));
		UT_ASSERT(c.end() == pmem_exp::end(c));
		UT_ASSERT(c.cend() == pmem_exp::cend(c));
		UT_ASSERT(static_cast<const C &>(c).end() == pmem_exp::cend(c));

		UT_ASSERT(c.rbegin() == pmem_exp::rbegin(c));
		UT_ASSERT(c.crbegin() == pmem_exp::crbegin(c));
		UT_ASSERT(c.rend() == pmem_exp::rend(c));
		UT_ASSERT(c.crend() == pmem_exp::crend(c));
		UT_ASSERT(static_cast<const C &>(c).rend() ==
			  pmem_exp::crend(c));

		UT_ASSERT(pmem_exp::begin(c) != pmem_exp::end(c));
		UT_ASSERT(pmem_exp::rbegin(c) != pmem_exp::rend(c));
		UT_ASSERT(pmem_exp::cbegin(c) != pmem_exp::cend(c));
		UT_ASSERT(pmem_exp::crbegin(c) != pmem_exp::crend(c));

		UT_ASSERT(*c.begin() == 0);
		UT_ASSERT(*c.rbegin() == 4);
		UT_ASSERT(*static_cast<const C &>(c).begin() == 0);
		UT_ASSERT(*static_cast<const C &>(c).rbegin() == 4);
		UT_ASSERT(*(c.end() - 1) == 4);
		UT_ASSERT(*(c.rend() - 1) == 0);

		UT_ASSERT(*pmem_exp::begin(c) == 0);
		UT_ASSERT(*(pmem_exp::begin(c) + 1) == 1);
		UT_ASSERT(*pmem_exp::cbegin(c) == 0);
		UT_ASSERT(*(pmem_exp::cbegin(c) + 1) == 1);
		UT_ASSERT(*pmem_exp::rbegin(c) == 4);
		UT_ASSERT(*pmem_exp::crbegin(c) == 4);

		UT_ASSERT(*pmem_exp::begin(static_cast<const C &>(c)) == 0);
		UT_ASSERT(*(pmem_exp::begin(static_cast<const C &>(c)) + 1) ==
			  1);
		UT_ASSERT(*pmem_exp::rbegin(static_cast<const C &>(c)) == 4);
		UT_ASSERT(*(pmem_exp::rbegin(static_cast<const C &>(c)) + 1) ==
			  3);
		UT_ASSERT(*(pmem_exp::end(static_cast<const C &>(c)) - 1) == 4);
		UT_ASSERT(*(pmem_exp::rend(static_cast<const C &>(c)) - 1) ==
			  0);
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

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(path, "iterators.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	return 0;
}
