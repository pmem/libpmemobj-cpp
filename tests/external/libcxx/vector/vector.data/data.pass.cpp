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

#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using vector_type = pmem::obj::vector<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v1;
	nvobj::persistent_ptr<vector_type> v2;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: data.pass",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<vector_type>();
			r->v2 = nvobj::make_persistent<vector_type>(100U);
			UT_ASSERT(r->v1->data() == 0);
			UT_ASSERT(r->v2->data() ==
				  std::addressof(r->v2->front()));
			nvobj::delete_persistent<vector_type>(r->v1);
			nvobj::delete_persistent<vector_type>(r->v2);
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
