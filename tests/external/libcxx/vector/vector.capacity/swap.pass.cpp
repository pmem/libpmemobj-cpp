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
#include <vector>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

using C = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<C> v1;
	nvobj::persistent_ptr<C> v2;
};

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
		path, "VectorTest: swap", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<C>(100U);
			r->v2 = nvobj::make_persistent<C>(200U);
		});

		r->v1->swap(*r->v2);

		UT_ASSERT(r->v1->size() == 200);
		UT_ASSERT(r->v1->capacity() == 200);
		UT_ASSERT(r->v2->size() == 100);
		UT_ASSERT(r->v2->capacity() == 100);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->v1);
			nvobj::delete_persistent<C>(r->v2);
		});

	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
