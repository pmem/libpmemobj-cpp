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
using vector_type = container_t<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v;
};

void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v = nvobj::make_persistent<vector_type>(
				std::initializer_list<int>{3, 4, 5, 6});
		});

		UT_ASSERT(r->v->size() == 4);
		UT_ASSERT((*r->v)[0] == 3);
		UT_ASSERT((*r->v)[1] == 4);
		UT_ASSERT((*r->v)[2] == 5);
		UT_ASSERT((*r->v)[3] == 6);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<vector_type>(r->v);
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
		path, "VectorTest: initializer_list.pass", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

	test(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
