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
};

template <class C>
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<C>(3U, 2);
			r->v2 = nvobj::make_persistent<C>(*r->v1);
		});

		*r->v2 = *r->v1;
		UT_ASSERT(*r->v2 == *r->v1);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->v1);
			nvobj::delete_persistent<C>(r->v2);
		});
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
		path, "VectorTest: assign_copy.pass", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

	test<C>(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
