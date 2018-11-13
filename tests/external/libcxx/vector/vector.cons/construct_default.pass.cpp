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

#include "libpmemobj++/experimental/vector.hpp"
#include "libpmemobj++/make_persistent.hpp"
#include "libpmemobj++/pool.hpp"
#include "libpmemobj++/transaction.hpp"
#include <cstring>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

struct foo {
	pmem_exp::vector<double> v_1;
	pmem_exp::vector<double> v_2 = {};
};

struct root {
	nvobj::persistent_ptr<pmem_exp::vector<int>> v_pptr;
	nvobj::persistent_ptr<foo> foo_pptr;
};

/**
 * Test pmem::obj::experimental::vector default constor.
 *
 * First case: call default constructor in three different ways and check if
 * new cointener is empty. Expect no exception was thrown.
 *
 * Second case: call default constructor for volatile instance of
 * pmem::obj::experimental::vector. Expect pmem::pool_error exception was
 * thrown.
 */
void
test_default_ctor(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();
	/* first case */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr =
				nvobj::make_persistent<pmem_exp::vector<int>>();
			r->foo_pptr = nvobj::make_persistent<foo>();
		});
		UT_ASSERT(r->v_pptr->empty() == 1);
		UT_ASSERT(r->foo_pptr->v_1.empty() == 1);
		UT_ASSERT(r->foo_pptr->v_2.empty() == 1);
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl
			  << std::strerror(nvobj::transaction::error())
			  << std::endl;
		UT_ASSERT(0);
	}

	/* second case */
	bool exception_thrown = false;
	try {
		pmem_exp::vector<int> v_3 = {};
		(void)v_3;
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);
}

int
main(int argc, char *argv[])
{
	START();
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: construct_default.pass", PMEMOBJ_MIN_POOL,
		S_IWUSR | S_IRUSR);
	test_default_ctor(pop);

	pop.close();

	return 0;
}
