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

#include "helper_classes.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

using vector_type = pmem_exp::vector<int>;
using vector_type2 = pmem_exp::vector<default_constructible_only>;

struct root {
	nvobj::persistent_ptr<vector_type> test1;
	nvobj::persistent_ptr<vector_type2> test2;
};

/**
 * Test pmem::obj::experimental::vector fill constructor
 *
 * Constructs container with n default constructed elements.
 * Validates container's size and its elements for both fundamental and user
 * defined types. Expects no exception is thrown.
 */
template <class C>
void
test(nvobj::pool<struct root> &pop, nvobj::persistent_ptr<C> &pptr,
     typename C::size_type n)
{
	/* construct */
	try {
		nvobj::transaction::run(
			pop, [&] { pptr = nvobj::make_persistent<C>(n); });
	} catch (std::exception &e) {
		UT_ASSERTexc(0, e);
	}
	/* validate */
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERTeq(pptr->size(), n);

			for (typename C::const_iterator i = pptr->begin(),
							e = pptr->end();
			     i != e; ++i)
				UT_ASSERT(*i == typename C::value_type());

			nvobj::delete_persistent<C>(pptr);
		});
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
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: construct_size",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	auto r = pop.root();

	test<vector_type>(pop, r->test1, 50);

	test<vector_type2>(pop, r->test2, 500);
	UT_ASSERT(default_constructible_only::count == 0);

	pop.close();

	return 0;
}
