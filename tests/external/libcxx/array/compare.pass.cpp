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

#include <vector>

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem_exp = pmem::obj::experimental;

template <class Array>
void
test_compare(const Array &LHS, const Array &RHS)
{
	typedef std::vector<typename Array::value_type> Vector;
	const Vector LHSV(LHS.begin(), LHS.end());
	const Vector RHSV(RHS.begin(), RHS.end());
	UT_ASSERT((LHS == RHS) == (LHSV == RHSV));
	UT_ASSERT((LHS != RHS) == (LHSV != RHSV));
	UT_ASSERT((LHS < RHS) == (LHSV < RHSV));
	UT_ASSERT((LHS <= RHS) == (LHSV <= RHSV));
	UT_ASSERT((LHS > RHS) == (LHSV > RHSV));
	UT_ASSERT((LHS >= RHS) == (LHSV >= RHSV));
}

struct Testcase1 {
	typedef int T;
	typedef pmem_exp::array<T, 3> C;
	C c1 = {{1, 2, 3}};
	C c2 = {{1, 2, 3}};
	C c3 = {{3, 2, 1}};
	C c4 = {{1, 2, 1}};

	void
	run()
	{
		test_compare(c1, c2);
		test_compare(c1, c3);
		test_compare(c1, c4);
	}
};

struct Testcase2 {
	typedef int T;
	typedef pmem_exp::array<T, 0> C;
	C c1 = {{}};
	C c2 = {{}};

	void
	run()
	{
		test_compare(c1, c2);
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

		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1->run();
			pop.root()->r2->run();
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
		pop = pmem::obj::pool<root>::create(path, "compare.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	return 0;
}
