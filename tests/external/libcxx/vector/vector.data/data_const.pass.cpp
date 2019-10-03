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
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using v_type_1 = pmem::obj::vector<int>;
using v_type_2 = pmem::obj::vector<failing_reference_operator>;

struct root {
	nvobj::persistent_ptr<const v_type_1> v1;
	nvobj::persistent_ptr<const v_type_1> v2;
	nvobj::persistent_ptr<const v_type_2> v3;
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
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: data_const.pass",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<v_type_1>();
			r->v2 = nvobj::make_persistent<v_type_1>(100U);
			r->v3 = nvobj::make_persistent<v_type_2>(100U);
		});
		UT_ASSERT(r->v1->data() == 0);
		UT_ASSERT(r->v2->data() == std::addressof(r->v2->front()));
		UT_ASSERT(r->v3->data() == std::addressof(r->v3->front()));
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<const v_type_1>(r->v1);
			nvobj::delete_persistent<const v_type_1>(r->v2);
			nvobj::delete_persistent<const v_type_2>(r->v3);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
