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

#include "helper_classes.hpp"
#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using C = container_t<emplace_constructible_moveable_and_assignable<int>>;

struct root {
	nvobj::persistent_ptr<C> v;
};

static void
test_emplaceable_concept(nvobj::pool<struct root> &pop)
{
	int arr1[] = {42};
	int arr2[] = {1, 101, 42};

	auto r = pop.root();
	/* TEST_1 */
	{
		using It = test_support::forward_it<int *>;

		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>();
			});

			r->v->assign(It(arr1), It(std::end(arr1)));
			UT_ASSERT((*r->v)[0].value == 42);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}

		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>();
			});

			r->v->assign(It(arr2), It(std::end(arr2)));
			UT_ASSERT((*r->v)[0].value == 1);
			UT_ASSERT((*r->v)[1].value == 101);
			UT_ASSERT((*r->v)[2].value == 42);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	/* TEST_2 */
	{
		using It = test_support::forward_it<int *>;

		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>();
			});

			r->v->assign(It(arr1), It(std::end(arr1)));
			UT_ASSERT((*r->v)[0].value == 42);
			UT_ASSERT((*r->v)[0].moved == 0);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}

		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>();
			});

			r->v->assign(It(arr2), It(std::end(arr2)));
			UT_ASSERT((*r->v)[0].value == 1);
			UT_ASSERT((*r->v)[1].value == 101);
			UT_ASSERT((*r->v)[2].value == 42);
			UT_ASSERT((*r->v)[2].moved == 0);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
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
		nvobj::pool<root>::create(path, "VectorTest: assign_iter_iter",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_emplaceable_concept(pop);

	pop.close();

	return 0;
}
