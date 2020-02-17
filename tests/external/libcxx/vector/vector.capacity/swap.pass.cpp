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
#include <vector>

namespace nvobj = pmem::obj;

using C = container_t<int>;

struct root {
	nvobj::persistent_ptr<C> v1;
	nvobj::persistent_ptr<C> v2;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VectorTest: swap",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<C>(100U);
			r->v2 = nvobj::make_persistent<C>(200U);
		});

		r->v1->swap(*r->v2);

		UT_ASSERT(r->v1->size() == 200);
		UT_ASSERT(r->v1->capacity() == expected_capacity<size_t>(200));
		UT_ASSERT(r->v2->size() == 100);
		UT_ASSERT(r->v2->capacity() == expected_capacity<size_t>(100));

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->v1);
			nvobj::delete_persistent<C>(r->v2);
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
