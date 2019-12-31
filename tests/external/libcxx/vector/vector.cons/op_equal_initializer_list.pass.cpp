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

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
using C = container_t<int>;

struct root {
	nvobj::persistent_ptr<C> d;
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
		path, "VectorTest: op_equal_initializer_list.pass",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->d = nvobj::make_persistent<C>(); });
		*r->d = {3, 4, 5, 6};

		UT_ASSERT(r->d->size() == 4);
		UT_ASSERT((*r->d)[0] == 3);
		UT_ASSERT((*r->d)[1] == 4);
		UT_ASSERT((*r->d)[2] == 5);
		UT_ASSERT((*r->d)[3] == 6);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->d); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
