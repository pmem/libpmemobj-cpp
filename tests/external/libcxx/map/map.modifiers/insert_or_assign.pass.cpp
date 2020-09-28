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

// template <class M>
//  pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj); // C++17
// template <class M>
//  pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj); // C++17
// template <class M>
//  iterator insert_or_assign(const_iterator hint, const key_type& k, M&& obj);
//  // C++17
// template <class M>
//  iterator insert_or_assign(const_iterator hint, key_type&& k, M&& obj); //
//  C++17

#include "../is_transparent.h"
#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <string>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container = container_t<int, Moveable, TRANSPARENT_COMPARE>;
using container2 = container_t<Moveable, Moveable, TRANSPARENT_COMPARE>;
using container3 = container_t<C2Int, Moveable, TRANSPARENT_COMPARE>;
using container4 =
	container_t<nvobj::string, Moveable, TRANSPARENT_COMPARE_STRING>;
using container5 =
	container_t<MoveableWrapper, MoveableWrapper, TRANSPARENT_COMPARE>;

struct root {
	nvobj::persistent_ptr<container> s;
	nvobj::persistent_ptr<container2> s2;
	nvobj::persistent_ptr<container3> s3;
	nvobj::persistent_ptr<container4> s4;
	nvobj::persistent_ptr<container5> s5;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{ // pair<iterator, bool> insert_or_assign(const key_type& k, M&& obj);
		typedef container M;
		typedef std::pair<M::iterator, bool> R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		R r;

		for (int i = 0; i < 20; i += 2)
			m.emplace(i, Moveable(i, (double)i));
		UT_ASSERT(m.size() == 10);

		for (int i = 0; i < 20; i += 2) {
			Moveable mv(i + 1, i + 1);
			r = m.insert_or_assign(i, std::move(mv));
			UT_ASSERT(m.size() == 10);
			UT_ASSERT(!r.second);		  // was not inserted
			UT_ASSERT(mv.moved());		  // was moved from
			UT_ASSERT(r.first->MAP_KEY == i); // key
			UT_ASSERT(r.first->MAP_VALUE.get() == i + 1); // value
		}

		Moveable mv1(5, 5.0);
		r = m.insert_or_assign(-1, std::move(mv1));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(r.second);			  // was inserted
		UT_ASSERT(mv1.moved());			  // was moved from
		UT_ASSERT(r.first->MAP_KEY == -1);	  // key
		UT_ASSERT(r.first->MAP_VALUE.get() == 5); // value

		Moveable mv2(9, 9.0);
		r = m.insert_or_assign(3, std::move(mv2));
		UT_ASSERT(m.size() == 12);
		UT_ASSERT(r.second);			  // was inserted
		UT_ASSERT(mv2.moved());			  // was moved from
		UT_ASSERT(r.first->MAP_KEY == 3);	  // key
		UT_ASSERT(r.first->MAP_VALUE.get() == 9); // value

		Moveable mv3(-1, 5.0);
		r = m.insert_or_assign(117, std::move(mv3));
		UT_ASSERT(m.size() == 13);
		UT_ASSERT(r.second);			   // was inserted
		UT_ASSERT(mv3.moved());			   // was moved from
		UT_ASSERT(r.first->MAP_KEY == 117);	   // key
		UT_ASSERT(r.first->MAP_VALUE.get() == -1); // value

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
	{ // pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj);
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
		r = m.insert_or_assign(std::move(mvkey1), std::move(mv1));
		UT_ASSERT(m.size() == 10);
		UT_ASSERT(!r.second);			  // was not inserted
		UT_ASSERT(!mvkey1.moved());		  // was not moved from
		UT_ASSERT(mv1.moved());			  // was moved from
		UT_ASSERT(r.first->MAP_KEY == mvkey1);	  // key
		UT_ASSERT(r.first->MAP_VALUE.get() == 4); // value

		Moveable mvkey2(3, 3.0);
		Moveable mv2(5, 5.0);
		r = m.insert_or_assign(std::move(mvkey2), std::move(mv2));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(r.second);			  // was inserted
		UT_ASSERT(mv2.moved());			  // was moved from
		UT_ASSERT(mvkey2.moved());		  // was moved from
		UT_ASSERT(r.first->MAP_KEY.get() == 3);	  // key
		UT_ASSERT(r.first->MAP_VALUE.get() == 5); // value

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
#ifdef XXX // XXX: implement insert_or_assign with hint
	{  // iterator insert_or_assign(const_iterator hint, const key_type& k,
	   // M&& obj);
		typedef container3 M;

		pmem::obj::transaction::run(
			pop, [&] { robj->s3 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s3;
		M::iterator r;

		for (int i = 0; i < 20; i += 2)
			m.emplace(i, Moveable(i, (double)i));
		UT_ASSERT(m.size() == 10);
		M::const_iterator it = m.find(C2Int{2});

		Moveable mv1(3, 3.0);
		C2Int k1{2};
		r = m.insert_or_assign(it, k1, std::move(mv1));
		UT_ASSERT(m.size() == 10);
		UT_ASSERT(mv1.moved());		    // was moved from
		UT_ASSERT(r->MAP_KEY.get() == 2);   // key
		UT_ASSERT(r->MAP_VALUE.get() == 3); // value

		Moveable mv2(5, 5.0);
		C2Int k2{3};
		r = m.insert_or_assign(it, k2, std::move(mv2));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(mv2.moved());		    // was moved from
		UT_ASSERT(r->MAP_KEY.get() == 3);   // key
		UT_ASSERT(r->MAP_VALUE.get() == 5); // value

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s3); });
	}
	{ // iterator insert_or_assign(const_iterator hint, key_type&& k, M&&
	  // obj);
		typedef container2 M;

		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s2;
		M::iterator r;

		for (int i = 0; i < 20; i += 2)
			m.emplace(Moveable(i, (double)i),
				  Moveable(i + 1, (double)i + 1));
		UT_ASSERT(m.size() == 10);
		M::const_iterator it = std::next(m.cbegin());

		Moveable mvkey1(2, 2.0);
		Moveable mv1(4, 4.0);
		r = m.insert_or_assign(it, std::move(mvkey1), std::move(mv1));
		UT_ASSERT(m.size() == 10);
		UT_ASSERT(mv1.moved());		    // was moved from
		UT_ASSERT(!mvkey1.moved());	    // was not moved from
		UT_ASSERT(r->MAP_KEY == mvkey1);    // key
		UT_ASSERT(r->MAP_VALUE.get() == 4); // value

		Moveable mvkey2(3, 3.0);
		Moveable mv2(5, 5.0);
		r = m.insert_or_assign(it, std::move(mvkey2), std::move(mv2));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(mv2.moved());		    // was moved from
		UT_ASSERT(mvkey2.moved());	    // was moved from
		UT_ASSERT(r->MAP_KEY.get() == 3);   // key
		UT_ASSERT(r->MAP_VALUE.get() == 5); // value

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
#endif
	{ // pair<iterator, bool> insert_or_assign(K &&k, M &&obj);
		typedef container3 M;
		typedef std::pair<M::iterator, bool> R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s3 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s3;
		R r;

		for (int i = 0; i < 10; i++)
			m.emplace(C2Int{i}, Moveable{i, 20.0});
		UT_ASSERT(m.size() == 10);

		for (int i = 0; i < 10; i++) {
			Moveable mv{i + 1, 10.0};
			r = m.insert_or_assign(i, std::move(mv));
			UT_ASSERT(!r.second);
			UT_ASSERT(r.first->MAP_KEY.get() == i);
			UT_ASSERT(r.first->MAP_VALUE.get() == i + 1);
			UT_ASSERT(mv.moved());
			UT_ASSERT(m.size() == 10);
		}

		for (int i = 10; i < 20; i++) {
			Moveable mv{i, 10.0};
			r = m.insert_or_assign(i, std::move(mv));
			UT_ASSERT(r.second);
			UT_ASSERT(r.first->MAP_KEY.get() == i);
			UT_ASSERT(r.first->MAP_VALUE.get() == i);
			UT_ASSERT(mv.moved());
		}
		UT_ASSERT(m.size() == 20);

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s3); });
	}
	{
		typedef container4 M;
		typedef std::pair<M::iterator, bool> R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s4 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s4;
		R r;

		UT_ASSERT(m.size() == 0);

		size_t j = 0;
		for (int i = 0; i < 10; i++) {
			Moveable mv{i, 10.0};
			auto key = std::string(j++, 'x');
			r = m.insert_or_assign(key, std::move(mv));
			UT_ASSERT(r.second);
			UT_ASSERT(r.first->MAP_KEY.compare(key) == 0);
			UT_ASSERT(r.first->MAP_VALUE.get() == i);
			UT_ASSERT(mv.moved());
			UT_ASSERT(m.size() == j);
		}

		j = 0;
		for (int i = 0; i < 10; i++) {
			Moveable mv{i + 1, 10.0};
			auto key = std::string(j++, 'x');
			r = m.insert_or_assign(key, std::move(mv));
			UT_ASSERT(!r.second);
			UT_ASSERT(r.first->MAP_KEY.compare(key) == 0);
			UT_ASSERT(r.first->MAP_VALUE.get() == i + 1);
			UT_ASSERT(mv.moved());
		}
		UT_ASSERT(m.size() == 10);

		j = 0;
		for (int i = 0; i < 10; i++) {
			Moveable mv{i + 2, 10.0};
			auto key = std::string(j++, 'x');
			r = m.insert_or_assign(std::move(key), std::move(mv));
			UT_ASSERT(!r.second);
			UT_ASSERT(r.first->MAP_KEY.compare(key) == 0);
			UT_ASSERT(r.first->MAP_VALUE.get() == i + 2);
			UT_ASSERT(mv.moved());
		}
		UT_ASSERT(m.size() == 10);

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s4); });
	}
	{
		typedef container5 M;
		typedef std::pair<M::iterator, bool> R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s5 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s5;
		R r;

		for (int i = 0; i < 20; i += 2)
			m.emplace(Moveable(i, (double)i),
				  Moveable(i + 1, (double)i + 1));
		UT_ASSERT(m.size() == 10);

		Moveable mvkey1(2, 2.0);
		Moveable mv1(4, 4.0);
		r = m.insert_or_assign(std::move(mvkey1), std::move(mv1));
		UT_ASSERT(m.size() == 10);
		UT_ASSERT(!r.second);	    // was not inserted
		UT_ASSERT(!mvkey1.moved()); // was not moved from
		UT_ASSERT(mv1.moved());	    // was moved from
		UT_ASSERT(r.first->MAP_KEY.get() == mvkey1);	// key
		UT_ASSERT(r.first->MAP_VALUE.get().get() == 4); // value

		Moveable mvkey2(3, 3.0);
		Moveable mv2(5, 5.0);
		r = m.insert_or_assign(std::move(mvkey2), std::move(mv2));
		UT_ASSERT(m.size() == 11);
		UT_ASSERT(r.second);			      // was inserted
		UT_ASSERT(mv2.moved());			      // was moved from
		UT_ASSERT(mvkey2.moved());		      // was moved from
		UT_ASSERT(r.first->MAP_KEY.get().get() == 3); // key
		UT_ASSERT(r.first->MAP_VALUE.get().get() == 5); // value

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s5); });
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
		pop = pmem::obj::pool<root>::create(
			path, "insert_or_assign.pass", PMEMOBJ_MIN_POOL,
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
