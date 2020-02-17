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

#include "helper_classes.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using C = pmem::obj::vector<move_only>;
using C2 = pmem::obj::vector<int>;

struct root {
	nvobj::persistent_ptr<C> l, lo, l2;
	nvobj::persistent_ptr<C2> c1, c2;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: move", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	{
		try {
			nvobj::transaction::run(pop, [&] {
				r->l = nvobj::make_persistent<C>(5U);
				r->lo = nvobj::make_persistent<C>(5U);
			});
			for (int i = 1; i <= 3; ++i) {
				r->l->push_back(i);
				r->lo->push_back(i);
			}

			nvobj::transaction::run(pop, [&] {
				r->l2 = nvobj::make_persistent<C>(
					std::move(*r->l));
			});
			UT_ASSERT(*r->l2 == *r->lo);
			UT_ASSERT(r->l->empty());

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->l);
				nvobj::delete_persistent<C>(r->lo);
				nvobj::delete_persistent<C>(r->l2);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		int a1[] = {1, 3, 7, 9, 10};
		try {
			nvobj::transaction::run(pop, [&] {
				r->c1 = nvobj::make_persistent<C2>(
					a1, a1 + sizeof(a1) / sizeof(a1[0]));
			});

			C2::const_iterator i = r->c1->begin();

			nvobj::transaction::run(pop, [&] {
				r->c2 = nvobj::make_persistent<C2>(
					std::move(*r->c1));
			});

			C2::iterator j = r->c2->erase(i);
			UT_ASSERT(*j == 3);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C2>(r->c1);
				nvobj::delete_persistent<C2>(r->c2);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
