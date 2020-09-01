//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Copyright 2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

// UNSUPPORTED: c++98, c++03

// <map>

// class map

// pair<iterator, bool> insert( value_type&& v);  // C++17 and later
// template <class P>
//   pair<iterator, bool> insert(P&& p);

#include "../is_transparent.h"
#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

class MoveOnly {
	MoveOnly(const MoveOnly &);
	MoveOnly &operator=(const MoveOnly &);

	int data_;

public:
	MoveOnly(int data = 1) : data_(data)
	{
	}
	MoveOnly(MoveOnly &&x) : data_(x.data_)
	{
		x.data_ = 0;
	}
	MoveOnly &
	operator=(MoveOnly &&x)
	{
		data_ = x.data_;
		x.data_ = 0;
		return *this;
	}

	int
	get() const
	{
		return data_;
	}

	bool
	operator==(const MoveOnly &x) const
	{
		return data_ == x.data_;
	}
	bool
	operator<(const MoveOnly &x) const
	{
		return data_ < x.data_;
	}
	MoveOnly
	operator+(const MoveOnly &x) const
	{
		return MoveOnly{data_ + x.data_};
	}
	MoveOnly operator*(const MoveOnly &x) const
	{
		return MoveOnly{data_ * x.data_};
	}
};

using container = container_t<int, MoveOnly>;
using container2 = container_t<C2Int, int, TRANSPARENT_COMPARE>;

struct root {
	nvobj::persistent_ptr<container> s;
	nvobj::persistent_ptr<container2> s2;
};

template <class Container, class Pair>
void
do_insert_rv_test(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	typedef Container M;
	typedef Pair P;
	typedef std::pair<typename M::iterator, bool> R;
	pmem::obj::transaction::run(
		pop, [&] { robj->s = nvobj::make_persistent<M>(); });
	auto &m = *robj->s;
	R r = m.insert(P(2, 2));
	UT_ASSERT(r.second);
	UT_ASSERT(r.first == m.begin());
	UT_ASSERT(m.size() == 1);
	UT_ASSERT(r.first->MAP_KEY == 2);
	UT_ASSERT(r.first->MAP_VALUE == 2);

	r = m.insert(P(1, 1));
	UT_ASSERT(r.second);
	UT_ASSERT(r.first == m.begin());
	UT_ASSERT(m.size() == 2);
	UT_ASSERT(r.first->MAP_KEY == 1);
	UT_ASSERT(r.first->MAP_VALUE == 1);

	r = m.insert(P(3, 3));
	UT_ASSERT(r.second);
	UT_ASSERT(m.size() == 3);
	UT_ASSERT(r.first->MAP_KEY == 3);
	UT_ASSERT(r.first->MAP_VALUE == 3);

	r = m.insert(P(3, 3));
	UT_ASSERT(!r.second);
	UT_ASSERT(m.size() == 3);
	UT_ASSERT(r.first->MAP_KEY == 3);
	UT_ASSERT(r.first->MAP_VALUE == 3);
	pmem::obj::transaction::run(
		pop, [&] { nvobj::delete_persistent<M>(robj->s); });
}

void
do_insert_rv_template_test(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	typedef container2 M;
	typedef std::pair<int, int> P;
	typedef std::pair<typename M::iterator, bool> R;

	pmem::obj::transaction::run(
		pop, [&] { robj->s2 = nvobj::make_persistent<M>(); });
	auto &m = *robj->s2;

	R r = m.insert(P(2, 2));
	UT_ASSERT(r.second);
	UT_ASSERT(r.first == m.begin());
	UT_ASSERT(m.size() == 1);
	UT_ASSERT(r.first->MAP_KEY.get() == 2);
	UT_ASSERT(r.first->MAP_VALUE == 2);

	r = m.insert(P(1, 1));
	UT_ASSERT(r.second);
	UT_ASSERT(r.first == m.begin());
	UT_ASSERT(m.size() == 2);
	UT_ASSERT(r.first->MAP_KEY.get() == 1);
	UT_ASSERT(r.first->MAP_VALUE == 1);

	r = m.insert(P(3, 3));
	UT_ASSERT(r.second);
	UT_ASSERT(m.size() == 3);
	UT_ASSERT(r.first->MAP_KEY.get() == 3);
	UT_ASSERT(r.first->MAP_VALUE == 3);

	r = m.insert(P(3, 3));
	UT_ASSERT(!r.second);
	UT_ASSERT(m.size() == 3);
	UT_ASSERT(r.first->MAP_KEY.get() == 3);
	UT_ASSERT(r.first->MAP_VALUE == 3);
	pmem::obj::transaction::run(
		pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
}

int
run(pmem::obj::pool<root> &pop)
{
	do_insert_rv_test<container, std::pair<int, MoveOnly>>(pop);
	do_insert_rv_test<container, std::pair<const int, MoveOnly>>(pop);

	do_insert_rv_template_test(pop);

#ifdef XXX // XXX: Implement min_allocator class
	{
		typedef std::map<int, MoveOnly, std::less<int>,
				 min_allocator<std::pair<const int, MoveOnly>>>
			M;
		typedef std::pair<int, MoveOnly> P;
		typedef std::pair<const int, MoveOnly> CP;
		do_insert_rv_test<M, P>();
		do_insert_rv_test<M, CP>();
	}
#endif
	{
		auto robj = pop.root();
		typedef container M;
		typedef std::pair<M::iterator, bool> R;
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		R r = m.insert({2, MoveOnly(2)});
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(r.first->MAP_KEY == 2);
		UT_ASSERT(r.first->MAP_VALUE == 2);

		r = m.insert({1, MoveOnly(1)});
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(r.first->MAP_KEY == 1);
		UT_ASSERT(r.first->MAP_VALUE == 1);

		r = m.insert({3, MoveOnly(3)});
		UT_ASSERT(r.second);
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(r.first->MAP_KEY == 3);
		UT_ASSERT(r.first->MAP_VALUE == 3);

		r = m.insert({3, MoveOnly(3)});
		UT_ASSERT(!r.second);
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(r.first->MAP_KEY == 3);
		UT_ASSERT(r.first->MAP_VALUE == 3);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}

	return 0;
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(path, "insert_rv.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}
	try {
		run(pop);
		pop.close();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
