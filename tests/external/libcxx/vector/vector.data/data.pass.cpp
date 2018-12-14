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

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <cstring>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using v_type_1 = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<v_type_1> v1;
	nvobj::persistent_ptr<v_type_1> v2;
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
		nvobj::pool<root>::create(path, "VectorTest: data.pass",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<v_type_1>();
			r->v2 = nvobj::make_persistent<v_type_1>(100U);
			UT_ASSERT(r->v1->data() == 0);
			UT_ASSERT(r->v2->data() ==
				  std::addressof(r->v2->front()));
		});
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl
			  << std::strerror(nvobj::transaction::error())
			  << std::endl;
		UT_ASSERT(0);
	}

	pop.close();

	return 0;
}
