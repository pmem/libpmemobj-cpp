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

#include "../is_transparent.h"
#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container = container_t<int, Moveable, TRANSPARENT_COMPARE>;
using container2 = container_t<Moveable, Moveable, TRANSPARENT_COMPARE>;

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
			UT_ASSERT(!r.second);		  // was not inserted
			UT_ASSERT(!mv1.moved());	  // was not moved from
			UT_ASSERT(r.first->MAP_KEY == i); // key
		}

		r = m.try_emplace(-1, std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(r.second);			  // was inserted
		UT_ASSERT(mv1.moved());			  // was moved from
		UT_ASSERT(r.first->MAP_KEY == -1);	  // key
		UT_ASSERT(r.first->MAP_VALUE.get() == 3); // value

		Moveable mv2(5, 3.0);
		r = m.try_emplace(5, std::move(mv2));
		UT_ASSERT(m.size() == 12);
		UT_ASSERT(r.second);			  // was inserted
		UT_ASSERT(mv2.moved());			  // was moved from
		UT_ASSERT(r.first->MAP_KEY == 5);	  // key
		UT_ASSERT(r.first->MAP_VALUE.get() == 5); // value

		Moveable mv3(-1, 3.0);
		r = m.try_emplace(117, std::move(mv2));
		UT_ASSERT(m.size() == 13);
		UT_ASSERT(r.second);			   // was inserted
		UT_ASSERT(mv2.moved());			   // was moved from
		UT_ASSERT(r.first->MAP_KEY == 117);	   // key
		UT_ASSERT(r.first->MAP_VALUE.get() == -1); // value
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
		UT_ASSERT(!r.second);		       // was not inserted
		UT_ASSERT(!mv1.moved());	       // was not moved from
		UT_ASSERT(!mvkey1.moved());	       // was not moved from
		UT_ASSERT(r.first->MAP_KEY == mvkey1); // key

		Moveable mvkey2(3, 3.0);
		r = m.try_emplace(std::move(mvkey2), std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(r.second);			  // was inserted
		UT_ASSERT(mv1.moved());			  // was moved from
		UT_ASSERT(mvkey2.moved());		  // was moved from
		UT_ASSERT(r.first->MAP_KEY.get() == 3);	  // key
		UT_ASSERT(r.first->MAP_VALUE.get() == 4); // value
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
#ifdef LIBPMEMOBJ_CPP_TESTS_RADIX
	{ // iterator try_emplace(const_iterator hint, const key_type& k,
	  // Args&&... args);
		typedef container M;
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		// M::iterator r;
		for (int i = 0; i < 20; i += 2)
			m.try_emplace(i, Moveable(i, (double)i));
		UT_ASSERT(m.size() == 10);
		/* XXX: emplace_hint */
		/*M::const_iterator it = m.find(2);

		Moveable mv1(3, 3.0);
		for (int i = 0; i < 20; i += 2) {
			r = m.emplace_hint(it, i, std::move(mv1));
			UT_ASSERT(m.size() == 10);
			UT_ASSERT(!mv1.moved());	 // was not moved from
			UT_ASSERT(r->MAP_KEY == i);	 // key
			UT_ASSERT(r->MAP_VALUE.get() == i); // value
		}

		r = m.emplace_hint(it, 3, std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(mv1.moved());		 // was moved from
		UT_ASSERT(r->MAP_KEY == 3);	 // key
		UT_ASSERT(r->MAP_VALUE.get() == 3); // value*/
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}

	{ // iterator try_emplace(const_iterator hint, key_type&& k, Args&&...
	  // args);
		typedef container2 M;
		// M::iterator r;
		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s2;
		for (int i = 0; i < 20; i += 2)
			m.emplace(Moveable(i, (double)i),
				  Moveable(i + 1, (double)i + 1));
		UT_ASSERT(m.size() == 10);
		/* XXX: emplace_hint */
		/*M::const_iterator it = std::next(m.cbegin());

		Moveable mvkey1(2, 2.0);
		Moveable mv1(4, 4.0);
		r = m.emplace_hint(it, std::move(mvkey1), std::move(mv1));
		UT_ASSERT(m.size() == 10);
		UT_ASSERT(!mv1.moved());       // was not moved from
		UT_ASSERT(!mvkey1.moved());    // was not moved from
		UT_ASSERT(r->MAP_KEY == mvkey1); // key

		Moveable mvkey2(3, 3.0);
		r = m.emplace_hint(it, std::move(mvkey2), std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(mv1.moved());		 // was moved from
		UT_ASSERT(mvkey2.moved());	 // was moved from
		UT_ASSERT(r->MAP_KEY.get() == 3);	 // key
		UT_ASSERT(r->MAP_VALUE.get() == 4); // value*/
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
