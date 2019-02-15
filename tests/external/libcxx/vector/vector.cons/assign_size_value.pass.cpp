//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using C = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<C> v1;
	nvobj::persistent_ptr<C> v2;
	nvobj::persistent_ptr<C> v3;
};

bool
is6(int x)
{
	return x == 6;
}

/**
 * Test pmem::obj::experimental::vector fill version of assign method
 *
 * Replaces vector's contents with count copies of given value
 * Validates size and elements of the vector.
 */
template <typename Vec>
void
test(nvobj::pool<struct root> &pop, Vec &v)
{
	try {
		v.assign(5, 6);
		UT_ASSERT(v.size() == 5);
		std::all_of(v.begin(), v.end(), is6);
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
		path, "VectorTest: assign_size_value.pass", PMEMOBJ_MIN_POOL,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<C>();
			r->v2 = nvobj::make_persistent<C>();
			r->v3 = nvobj::make_persistent<C>(10U, 1);
		});

		r->v2->reserve(10); // no reallocation during assign

		test<C>(pop, *r->v1);
		test<C>(pop, *r->v2);
		test<C>(pop, *r->v3);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->v1);
			nvobj::delete_persistent<C>(r->v2);
			nvobj::delete_persistent<C>(r->v3);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
