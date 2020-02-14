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

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using C = container_t<int>;

struct root {
	nvobj::persistent_ptr<C> c;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VectorTest: push_back",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->c = nvobj::make_persistent<C>(); });
		r->c->push_back(0);
		UT_ASSERT(r->c->size() == 1);
		for (int j = 0; static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[static_cast<C::size_type>(j)] == j);
		r->c->push_back(1);
		UT_ASSERT(r->c->size() == 2);
		for (int j = 0; static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[static_cast<C::size_type>(j)] == j);
		r->c->push_back(2);
		UT_ASSERT(r->c->size() == 3);
		for (int j = 0; static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[static_cast<C::size_type>(j)] == j);
		r->c->push_back(3);
		UT_ASSERT(r->c->size() == 4);
		for (int j = 0; static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[static_cast<C::size_type>(j)] == j);
		r->c->push_back(4);
		UT_ASSERT(r->c->size() == 5);
		for (int j = 0; static_cast<std::size_t>(j) < r->c->size(); ++j)
			UT_ASSERT((*r->c)[static_cast<C::size_type>(j)] == j);
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->c); });
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
