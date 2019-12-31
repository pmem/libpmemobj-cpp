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

using C = pmem::obj::vector<move_only>;

struct root {
	nvobj::persistent_ptr<C> c;
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
		nvobj::pool<root>::create(path, "VectorTest: push_back_rvalue",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->c = nvobj::make_persistent<C>(); });
		r->c->push_back(move_only(0));
		UT_ASSERT(r->c->size() == 1);
		for (std::size_t j = 0;
		     static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[j] == move_only(j));
		r->c->push_back(move_only(1));
		UT_ASSERT(r->c->size() == 2);
		for (std::size_t j = 0;
		     static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[j] == move_only(j));
		r->c->push_back(move_only(2));
		UT_ASSERT(r->c->size() == 3);
		for (std::size_t j = 0;
		     static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[j] == move_only(j));
		r->c->push_back(move_only(3));
		UT_ASSERT(r->c->size() == 4);
		for (std::size_t j = 0;
		     static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[j] == move_only(j));
		r->c->push_back(move_only(4));
		UT_ASSERT(r->c->size() == 5);
		for (std::size_t j = 0;
		     static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[j] == move_only(j));
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->c); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
