//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
using C = container_t<int>;

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
 * Test pmem::obj::vector fill version of assign method
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

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: assign_size_value.pass",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

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
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
