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
	nvobj::persistent_ptr<C> v;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VectorTest: size",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v = nvobj::make_persistent<C>(); });
		UT_ASSERT(r->v->size() == 0);

		r->v->push_back(C::value_type(2));
		UT_ASSERT(r->v->size() == 1);

		r->v->push_back(C::value_type(1));
		UT_ASSERT(r->v->size() == 2);

		r->v->push_back(C::value_type(3));
		UT_ASSERT(r->v->size() == 3);

		r->v->erase(r->v->begin());
		UT_ASSERT(r->v->size() == 2);

		r->v->erase(r->v->begin());
		UT_ASSERT(r->v->size() == 1);

		r->v->erase(r->v->begin());
		UT_ASSERT(r->v->size() == 0);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v); });

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
