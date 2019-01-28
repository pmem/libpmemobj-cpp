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

using C = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<C> v;
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
		path, "VectorTest: size", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v = nvobj::make_persistent<C>(); });
		UT_ASSERT(r->v->size() == 0);

		nvobj::transaction::run(
			pop, [&] { r->v->push_back(C::value_type(2)); });
		UT_ASSERT(r->v->size() == 1);

		nvobj::transaction::run(
			pop, [&] { r->v->push_back(C::value_type(1)); });
		UT_ASSERT(r->v->size() == 2);

		nvobj::transaction::run(
			pop, [&] { r->v->push_back(C::value_type(3)); });
		UT_ASSERT(r->v->size() == 3);

		nvobj::transaction::run(pop,
					[&] { r->v->erase(r->v->begin()); });
		UT_ASSERT(r->v->size() == 2);

		nvobj::transaction::run(pop,
					[&] { r->v->erase(r->v->begin()); });
		UT_ASSERT(r->v->size() == 1);

		nvobj::transaction::run(pop,
					[&] { r->v->erase(r->v->begin()); });
		UT_ASSERT(r->v->size() == 0);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v); });

	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
