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
	nvobj::persistent_ptr<C> v;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: insert_iter_size_value",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	{
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>(100U);
			});
			C::iterator i = r->v->insert(r->v->cbegin() + 10, 5, 1);
			UT_ASSERT(r->v->size() == 105);
			UT_ASSERT(i == r->v->begin() + 10);
			unsigned j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			for (; j < 15; ++j)
				UT_ASSERT((*r->v)[j] == 1);
			for (++j; j < 105; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>(100U);
			});
			while (r->v->size() < r->v->capacity())
				r->v->push_back(0); // force reallocation
			size_t sz = r->v->size();
			C::iterator i = r->v->insert(r->v->cbegin() + 10, 5, 1);
			UT_ASSERT(r->v->size() == sz + 5);
			UT_ASSERT(i == r->v->begin() + 10);
			std::size_t j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			for (; j < 15; ++j)
				UT_ASSERT((*r->v)[j] == 1);
			for (++j; j < r->v->size(); ++j)
				UT_ASSERT((*r->v)[j] == 0);
			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>(100U);
			});
			r->v->reserve(128); // force no reallocation
			size_t sz = r->v->size();
			C::iterator i = r->v->insert(r->v->cbegin() + 10, 5, 1);
			UT_ASSERT(r->v->size() == sz + 5);
			UT_ASSERT(i == r->v->begin() + 10);
			std::size_t j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			for (; j < 15; ++j)
				UT_ASSERT((*r->v)[j] == 1);
			for (++j; j < r->v->size(); ++j)
				UT_ASSERT((*r->v)[j] == 0);
			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
