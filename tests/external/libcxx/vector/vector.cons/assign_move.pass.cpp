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

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using C = pmem_exp::vector<move_only>;

struct root {
	nvobj::persistent_ptr<C> l;
	nvobj::persistent_ptr<C> lo;
	nvobj::persistent_ptr<C> l2;
};

template <typename C>
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->l = nvobj::make_persistent<C>();
			r->lo = nvobj::make_persistent<C>();
			r->l2 = nvobj::make_persistent<C>();
		});

		for (int i = 1; i <= 3; ++i) {
			r->l->push_back(i);
			r->lo->push_back(i);
		}
		*r->l2 = std::move(*r->l);

		UT_ASSERT(*r->l2 == *r->lo);
		UT_ASSERT(r->l->empty());

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->l);
			nvobj::delete_persistent<C>(r->lo);
			nvobj::delete_persistent<C>(r->l2);
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
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: assign_move.pass",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test<C>(pop);

	pop.close();

	return 0;
}
