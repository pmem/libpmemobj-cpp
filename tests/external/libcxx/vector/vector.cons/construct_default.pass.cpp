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

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <cstring>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using vector_type = pmem_exp::vector<int>;

struct foo {
	vector_type v_1;
#ifdef NO_CLANG_BRACE_INITIALIZATION_NEWEXPR_BUG
	vector_type v_2 = {};
#endif
};

struct root {
	nvobj::persistent_ptr<vector_type> v_pptr;
	nvobj::persistent_ptr<foo> foo_pptr;
};

/**
 * Test pmem::obj::experimental::vector default constructor.
 *
 * First case: call default constructor in three different ways and check if
 * new container is empty. Expect no exception is thrown.
 */
void
test_default_ctor(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();
	try {
		nvobj::transaction::run(pop, [&] {
			r->v_pptr = nvobj::make_persistent<vector_type>();
			r->foo_pptr = nvobj::make_persistent<foo>();
		});
		UT_ASSERT(r->v_pptr->empty() == 1);
		UT_ASSERT(r->foo_pptr->v_1.empty() == 1);
#ifdef NO_CLANG_BRACE_INITIALIZATION_NEWEXPR_BUG
		UT_ASSERT(r->foo_pptr->v_2.empty() == 1);
#endif
		nvobj::delete_persistent_atomic<vector_type>(r->v_pptr);
		nvobj::delete_persistent_atomic<foo>(r->foo_pptr);
	} catch (std::exception &e) {
		UT_ASSERTexc(0, e);
	}
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
