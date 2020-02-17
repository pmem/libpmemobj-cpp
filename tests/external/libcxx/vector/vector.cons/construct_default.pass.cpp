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

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>

namespace nvobj = pmem::obj;
using vector_type = container_t<int>;

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
 * Test pmem::obj::vector default constructor.
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
		UT_FATALexc(e);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: construct_default.pass",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);
	test_default_ctor(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
