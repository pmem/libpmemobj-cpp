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
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using vector_type = container_t<int>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr;
};

/**
 * Test pmem::obj::vector fill constructor
 *
 * Constructs container with n copies of elements with custom value.
 * Validates container's size and its elements. Expects no exception is thrown.
 */
template <class C>
void
test(nvobj::pool<struct root> &pop, typename C::size_type n)
{
	const int val = 3;

	auto r = pop.root();
	/* construct */
	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<C>(n, val);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	/* validate */
	try {
		UT_ASSERTeq(r->pptr->size(), n);

		for (typename C::const_iterator i = r->pptr->begin(),
						e = r->pptr->end();
		     i != e; ++i)
			UT_ASSERTeq(*i, val);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->pptr); });
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
		path, "VectorTest: construct_size_value", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

	test<vector_type>(pop, 5);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
