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

// <map>

// class map

// template <class... Args>
//  pair<iterator, bool> try_emplace(const key_type& k, Args&&... args); //
//  C++17
// template <class... Args>
//  pair<iterator, bool> try_emplace(key_type&& k, Args&&... args); // C++17
// template <class... Args>
//  iterator try_emplace(const_iterator hint, const key_type& k, Args&&...
//  args); // C++17
// template <class... Args>
//  iterator try_emplace(const_iterator hint, key_type&& k, Args&&... args); //
//  C++17

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

class Moveable {
	Moveable(const Moveable &);
	Moveable &operator=(const Moveable &);

	int int_;
	double double_;

public:
	Moveable() : int_(0), double_(0)
	{
	}
	Moveable(int i, double d) : int_(i), double_(d)
	{
	}
	Moveable(Moveable &&x) : int_(x.int_), double_(x.double_)
	{
		x.int_ = -1;
		x.double_ = -1;
	}
	Moveable &
	operator=(Moveable &&x)
	{
		int_ = x.int_;
		x.int_ = -1;
		double_ = x.double_;
		x.double_ = -1;
		return *this;
	}

	bool
	operator==(const Moveable &x) const
	{
		return int_ == x.int_ && double_ == x.double_;
	}
	bool
	operator<(const Moveable &x) const
	{
		return int_ < x.int_ || (int_ == x.int_ && double_ < x.double_);
	}

	int
	get() const
	{
		return int_;
	}
	bool
	moved() const
	{
		return int_ == -1;
	}
};

using container = container_t<int, Moveable>;
using container2 = container_t<Moveable, Moveable>;

struct root {
	nvobj::persistent_ptr<container> s;
	nvobj::persistent_ptr<container2> s2;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{ // pair<iterator, bool> try_emplace(const key_type& k, Args&&...
	  // args);
		typedef container M;
		typedef std::pair<M::iterator, bool> R;
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		R r;
		for (int i = 0; i < 20; i += 2)
			m.emplace(i, Moveable(i, (double)i));
		UT_ASSERT(m.size() == 10);

		Moveable mv1(3, 3.0);
		for (int i = 0; i < 20; i += 2) {
			r = m.try_emplace(i, std::move(mv1));
			UT_ASSERT(m.size() == 10);
			UT_ASSERT(!r.second);		// was not inserted
			UT_ASSERT(!mv1.moved());	// was not moved from
			UT_ASSERT(r.first->first == i); // key
		}

		r = m.try_emplace(-1, std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(r.second);		       // was inserted
		UT_ASSERT(mv1.moved());		       // was moved from
		UT_ASSERT(r.first->first == -1);       // key
		UT_ASSERT(r.first->second.get() == 3); // value

		Moveable mv2(5, 3.0);
		r = m.try_emplace(5, std::move(mv2));
		UT_ASSERT(m.size() == 12);
		UT_ASSERT(r.second);		       // was inserted
		UT_ASSERT(mv2.moved());		       // was moved from
		UT_ASSERT(r.first->first == 5);	       // key
		UT_ASSERT(r.first->second.get() == 5); // value

		Moveable mv3(-1, 3.0);
		r = m.try_emplace(117, std::move(mv2));
		UT_ASSERT(m.size() == 13);
		UT_ASSERT(r.second);			// was inserted
		UT_ASSERT(mv2.moved());			// was moved from
		UT_ASSERT(r.first->first == 117);	// key
		UT_ASSERT(r.first->second.get() == -1); // value
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}

	{ // pair<iterator, bool> try_emplace(key_type&& k, Args&&... args);
		typedef container2 M;
		typedef std::pair<M::iterator, bool> R;
		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s2;
		R r;
		for (int i = 0; i < 20; i += 2)
			m.emplace(Moveable(i, (double)i),
				  Moveable(i + 1, (double)i + 1));
		UT_ASSERT(m.size() == 10);

		Moveable mvkey1(2, 2.0);
		Moveable mv1(4, 4.0);
		r = m.try_emplace(std::move(mvkey1), std::move(mv1));
		UT_ASSERT(m.size() == 10);
		UT_ASSERT(!r.second);		     // was not inserted
		UT_ASSERT(!mv1.moved());	     // was not moved from
		UT_ASSERT(!mvkey1.moved());	     // was not moved from
		UT_ASSERT(r.first->first == mvkey1); // key

		Moveable mvkey2(3, 3.0);
		r = m.try_emplace(std::move(mvkey2), std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(r.second);		       // was inserted
		UT_ASSERT(mv1.moved());		       // was moved from
		UT_ASSERT(mvkey2.moved());	       // was moved from
		UT_ASSERT(r.first->first.get() == 3);  // key
		UT_ASSERT(r.first->second.get() == 4); // value
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
#ifdef XXX // XXX: There is no matching function for try_emplace(it, i,
	   // std::move(mv1));
	{  // iterator try_emplace(const_iterator hint, const key_type& k,
	   // Args&&... args);
		typedef container M;
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		M::iterator r;
		for (int i = 0; i < 20; i += 2)
			m.try_emplace(i, Moveable(i, (double)i));
		UT_ASSERT(m.size() == 10);
		M::const_iterator it = m.find(2);

		Moveable mv1(3, 3.0);
		for (int i = 0; i < 20; i += 2) {
			r = m.emplace_hint(it, i, std::move(mv1));
			UT_ASSERT(m.size() == 10);
			UT_ASSERT(!mv1.moved());	 // was not moved from
			UT_ASSERT(r->first == i);	 // key
			UT_ASSERT(r->second.get() == i); // value
		}

		r = m.emplace_hint(it, 3, std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(mv1.moved());		 // was moved from
		UT_ASSERT(r->first == 3);	 // key
		UT_ASSERT(r->second.get() == 3); // value
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}

	{ // iterator try_emplace(const_iterator hint, key_type&& k, Args&&...
	  // args);
		typedef container2 M;
		M::iterator r;
		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s2;
		for (int i = 0; i < 20; i += 2)
			m.emplace(Moveable(i, (double)i),
				  Moveable(i + 1, (double)i + 1));
		UT_ASSERT(m.size() == 10);
		M::const_iterator it = std::next(m.cbegin());

		Moveable mvkey1(2, 2.0);
		Moveable mv1(4, 4.0);
		r = m.emplace_hint(it, std::move(mvkey1), std::move(mv1));
		UT_ASSERT(m.size() == 10);
		UT_ASSERT(!mv1.moved());       // was not moved from
		UT_ASSERT(!mvkey1.moved());    // was not moved from
		UT_ASSERT(r->first == mvkey1); // key

		Moveable mvkey2(3, 3.0);
		r = m.emplace_hint(it, std::move(mvkey2), std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(mv1.moved());		 // was moved from
		UT_ASSERT(mvkey2.moved());	 // was moved from
		UT_ASSERT(r->first.get() == 3);	 // key
		UT_ASSERT(r->second.get() == 4); // value
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
#endif
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
		pop = pmem::obj::pool<root>::create(path, "try.emplace.pass",
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
