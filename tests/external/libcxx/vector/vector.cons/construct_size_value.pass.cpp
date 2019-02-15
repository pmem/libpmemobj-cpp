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

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

using vector_type = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr;
};

/**
 * Test pmem::obj::experimental::vector fill constructor
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
		path, "VectorTest: construct_size_value", PMEMOBJ_MIN_POOL,
		S_IWUSR | S_IRUSR);

	test<vector_type>(pop, 5);

	pop.close();

	return 0;
}
