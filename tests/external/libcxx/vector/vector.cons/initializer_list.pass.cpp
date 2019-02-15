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

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using vector_type = pmem_exp::vector<int>;

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
		path, "VectorTest: initializer_list.pass", PMEMOBJ_MIN_POOL,
		S_IWUSR | S_IRUSR);

	test(pop);

	pop.close();

	return 0;
}
