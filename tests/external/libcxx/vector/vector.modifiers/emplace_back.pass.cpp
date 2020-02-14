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

class A;

using C = container_t<A>;

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

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VectorTest: emplace_back",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->c = nvobj::make_persistent<C>(); });

		r->c->emplace_back((int64_t)2, 3.5);
		UT_ASSERT(r->c->size() == 1);
		UT_ASSERT(r->c->front().geti() == 2);
		UT_ASSERT(r->c->front().getd() == 3.5);
		r->c->emplace_back((int64_t)3, 4.5);
		UT_ASSERT(r->c->size() == 2);
		UT_ASSERT(r->c->front().geti() == 2);
		UT_ASSERT(r->c->front().getd() == 3.5);
		UT_ASSERT(r->c->back().geti() == 3);
		UT_ASSERT(r->c->back().getd() == 4.5);

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
