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

struct Throws;

using C = pmem_exp::vector<int>;

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
		path, "VectorTest: insert_iter_initializer_list",
		PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->d = nvobj::make_persistent<C>(10U, 1); });
		C::iterator i = r->d->insert(r->d->cbegin() + 2, {3, 4, 5, 6});
		UT_ASSERT(r->d->size() == 14);
		UT_ASSERT(i == r->d->begin() + 2);
		UT_ASSERT((*r->d)[0] == 1);
		UT_ASSERT((*r->d)[1] == 1);
		UT_ASSERT((*r->d)[2] == 3);
		UT_ASSERT((*r->d)[3] == 4);
		UT_ASSERT((*r->d)[4] == 5);
		UT_ASSERT((*r->d)[5] == 6);
		UT_ASSERT((*r->d)[6] == 1);
		UT_ASSERT((*r->d)[7] == 1);
		UT_ASSERT((*r->d)[8] == 1);
		UT_ASSERT((*r->d)[9] == 1);
		UT_ASSERT((*r->d)[10] == 1);
		UT_ASSERT((*r->d)[11] == 1);
		UT_ASSERT((*r->d)[12] == 1);
		UT_ASSERT((*r->d)[13] == 1);
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->d); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
