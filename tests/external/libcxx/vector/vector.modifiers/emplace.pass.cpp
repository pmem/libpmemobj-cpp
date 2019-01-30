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

class A;

using C = pmem_exp::vector<A>;

struct root {
	nvobj::persistent_ptr<C> c;
};

class A {
	int64_t i_;
	double d_;

	A(const A &);
	A &operator=(const A &);

public:
	A(int64_t i, double d) : i_(i), d_(d)
	{
	}

	A(A &&a) : i_(a.i_), d_(a.d_)
	{
		a.i_ = 0;
		a.d_ = 0;
	}

	A &
	operator=(A &&a)
	{
		i_ = a.i_;
		d_ = a.d_;
		a.i_ = 0;
		a.d_ = 0;
		return *this;
	}

	int
	geti() const
	{
		return i_;
	}
	double
	getd() const
	{
		return d_;
	}
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
		nvobj::pool<root>::create(path, "VectorTest: emplace",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->c = nvobj::make_persistent<C>(); });

		C::iterator i = r->c->emplace(r->c->cbegin(), (int64_t)2, 3.5);
		UT_ASSERT(i == r->c->begin());
		UT_ASSERT(r->c->size() == 1);
		UT_ASSERT(r->c->front().geti() == 2);
		UT_ASSERT(r->c->front().getd() == 3.5);
		i = r->c->emplace(r->c->cend(), (int64_t)3, 4.5);
		UT_ASSERT(i == r->c->end() - 1);
		UT_ASSERT(r->c->size() == 2);
		UT_ASSERT(r->c->front().geti() == 2);
		UT_ASSERT(r->c->front().getd() == 3.5);
		UT_ASSERT(r->c->back().geti() == 3);
		UT_ASSERT(r->c->back().getd() == 4.5);
		i = r->c->emplace(r->c->cbegin() + 1, (int64_t)4, 6.5);
		UT_ASSERT(i == r->c->begin() + 1);
		UT_ASSERT(r->c->size() == 3);
		UT_ASSERT(r->c->front().geti() == 2);
		UT_ASSERT(r->c->front().getd() == 3.5);
		UT_ASSERT((*r->c)[1].geti() == 4);
		UT_ASSERT((*r->c)[1].getd() == 6.5);
		UT_ASSERT(r->c->back().geti() == 3);
		UT_ASSERT(r->c->back().getd() == 4.5);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->c); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
