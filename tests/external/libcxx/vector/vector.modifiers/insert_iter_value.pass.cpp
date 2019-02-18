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
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: insert_iter_value",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	{
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C>(100U);
			});
			C::iterator i = r->v->insert(r->v->cbegin() + 10, 1);
			UT_ASSERT(r->v->size() == 101);
			UT_ASSERT(i == r->v->begin() + 10);
			unsigned j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
			UT_ASSERT((*r->v)[j] == 1);
			for (++j; j < 101; ++j)
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
			C::iterator i = r->v->insert(r->v->cbegin() + 10, 1);
			UT_ASSERT(r->v->size() == sz + 1);
			UT_ASSERT(i == r->v->begin() + 10);
			std::size_t j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
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
			while (r->v->size() < r->v->capacity())
				r->v->push_back(0);
			r->v->pop_back();
			r->v->pop_back(); // force no reallocation
			size_t sz = r->v->size();
			C::iterator i = r->v->insert(r->v->cbegin() + 10, 1);
			UT_ASSERT(r->v->size() == sz + 1);
			UT_ASSERT(i == r->v->begin() + 10);
			std::size_t j;
			for (j = 0; j < 10; ++j)
				UT_ASSERT((*r->v)[j] == 0);
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

	return 0;
}
