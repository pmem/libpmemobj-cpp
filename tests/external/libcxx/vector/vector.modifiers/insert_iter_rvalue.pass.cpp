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
#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using C = container_t<move_only>;

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
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: insert_iter_rvalue", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v = nvobj::make_persistent<C>(100U); });
		C::iterator i = r->v->insert(r->v->cbegin() + 10, move_only(3));
		UT_ASSERT(r->v->size() == 101);
		UT_ASSERT(i == r->v->begin() + 10);
		unsigned j;
		for (j = 0; j < 10; ++j)
			UT_ASSERT((*r->v)[j] == move_only());
		UT_ASSERT((*r->v)[j] == move_only(3));
		for (++j; j < 101; ++j)
			UT_ASSERT((*r->v)[j] == move_only());
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
