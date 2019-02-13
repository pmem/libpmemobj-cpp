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

#include <iterator>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

struct Throws;

using C = pmem_exp::vector<int>;
using C2 = pmem_exp::vector<Throws>;
using std::distance;
using std::next;

struct root {
	nvobj::persistent_ptr<C> l1;
	nvobj::persistent_ptr<C2> v;
};

struct Throws {
	Throws() : v_(0)
	{
	}
	Throws(int v) : v_(v)
	{
	}
	Throws(const Throws &rhs) : v_(rhs.v_)
	{
		if (sThrows)
			throw 1;
	}
	Throws(Throws &&rhs) : v_(rhs.v_)
	{
		if (sThrows)
			throw 1;
	}
	Throws &
	operator=(const Throws &rhs)
	{
		v_ = rhs.v_;
		return *this;
	}
	Throws &
	operator=(Throws &&rhs)
	{
		v_ = rhs.v_;
		return *this;
	}
	int v_;
	static bool sThrows;
};

bool Throws::sThrows = false;

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
		nvobj::pool<root>::create(path, "VectorTest: erase_iter",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		int a1[] = {1, 2, 3};
		try {
			nvobj::transaction::run(pop, [&] {
				r->l1 = nvobj::make_persistent<C>(a1, a1 + 3);
			});
			C::const_iterator i = r->l1->begin();
			++i;
			C::iterator j = r->l1->erase(i);
			UT_ASSERT(r->l1->size() == 2);
			UT_ASSERT(distance(r->l1->begin(), r->l1->end()) == 2);
			UT_ASSERT(*j == 3);
			UT_ASSERT(*r->l1->begin() == 1);
			UT_ASSERT(*std::next(r->l1->begin()) == 3);
			j = r->l1->erase(j);
			UT_ASSERT(j == r->l1->end());
			UT_ASSERT(r->l1->size() == 1);
			UT_ASSERT(distance(r->l1->begin(), r->l1->end()) == 1);
			UT_ASSERT(*r->l1->begin() == 1);
			j = r->l1->erase(r->l1->begin());
			UT_ASSERT(j == r->l1->end());
			UT_ASSERT(r->l1->size() == 0);
			UT_ASSERT(distance(r->l1->begin(), r->l1->end()) == 0);

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(r->l1);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		Throws arr[] = {1, 2, 3};
		try {
			nvobj::transaction::run(pop, [&] {
				r->v = nvobj::make_persistent<C2>(arr, arr + 3);
			});
			Throws::sThrows = true;
			r->v->erase(r->v->begin());
			r->v->erase(--r->v->end());
			r->v->erase(r->v->begin());
			UT_ASSERT(r->v->size() == 0);
			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C2>(r->v);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();

	return 0;
}
