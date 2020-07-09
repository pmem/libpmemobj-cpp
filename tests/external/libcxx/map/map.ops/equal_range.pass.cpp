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

// pair<iterator,iterator>             equal_range(const key_type& k);
// pair<const_iterator,const_iterator> equal_range(const key_type& k) const;

#include "unittest.hpp"

#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../is_transparent.h"
#include "../private_constructor.h"

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = nvobjex::concurrent_map<int, double>;
using C1 = nvobjex::concurrent_map<int, double, transparent_less>;
using C2 =
	nvobjex::concurrent_map<PrivateConstructor, double, transparent_less>;
struct root {
	nvobj::persistent_ptr<C> s;
	nvobj::persistent_ptr<C1> s1;
	nvobj::persistent_ptr<C2> s2;
};

int
run(pmem::obj::pool<root> &pop)
{
	{
		auto robj = pop.root();
		typedef std::pair<const int, double> V;
		typedef C M;
		{
			typedef std::pair<M::iterator, M::iterator> R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			pmem::obj::transaction::run(pop, [&] {
				robj->s = nvobj::make_persistent<C>(
					ar, ar + sizeof(ar) / sizeof(ar[0]));
			});
			auto &m = *robj->s;
			R r = m.equal_range(5);
			UT_ASSERT(r.first == next(m.begin(), 0));
			UT_ASSERT(r.second == next(m.begin(), 1));
			r = m.equal_range(7);
			UT_ASSERT(r.first == next(m.begin(), 1));
			UT_ASSERT(r.second == next(m.begin(), 2));
			r = m.equal_range(9);
			UT_ASSERT(r.first == next(m.begin(), 2));
			UT_ASSERT(r.second == next(m.begin(), 3));
			r = m.equal_range(11);
			UT_ASSERT(r.first == next(m.begin(), 3));
			UT_ASSERT(r.second == next(m.begin(), 4));
			r = m.equal_range(13);
			UT_ASSERT(r.first == next(m.begin(), 4));
			UT_ASSERT(r.second == next(m.begin(), 5));
			r = m.equal_range(15);
			UT_ASSERT(r.first == next(m.begin(), 5));
			UT_ASSERT(r.second == next(m.begin(), 6));
			r = m.equal_range(17);
			UT_ASSERT(r.first == next(m.begin(), 6));
			UT_ASSERT(r.second == next(m.begin(), 7));
			r = m.equal_range(19);
			UT_ASSERT(r.first == next(m.begin(), 7));
			UT_ASSERT(r.second == next(m.begin(), 8));
			r = m.equal_range(4);
			UT_ASSERT(r.first == next(m.begin(), 0));
			UT_ASSERT(r.second == next(m.begin(), 0));
			r = m.equal_range(6);
			UT_ASSERT(r.first == next(m.begin(), 1));
			UT_ASSERT(r.second == next(m.begin(), 1));
			r = m.equal_range(8);
			UT_ASSERT(r.first == next(m.begin(), 2));
			UT_ASSERT(r.second == next(m.begin(), 2));
			r = m.equal_range(10);
			UT_ASSERT(r.first == next(m.begin(), 3));
			UT_ASSERT(r.second == next(m.begin(), 3));
			r = m.equal_range(12);
			UT_ASSERT(r.first == next(m.begin(), 4));
			UT_ASSERT(r.second == next(m.begin(), 4));
			r = m.equal_range(14);
			UT_ASSERT(r.first == next(m.begin(), 5));
			UT_ASSERT(r.second == next(m.begin(), 5));
			r = m.equal_range(16);
			UT_ASSERT(r.first == next(m.begin(), 6));
			UT_ASSERT(r.second == next(m.begin(), 6));
			r = m.equal_range(18);
			UT_ASSERT(r.first == next(m.begin(), 7));
			UT_ASSERT(r.second == next(m.begin(), 7));
			r = m.equal_range(20);
			UT_ASSERT(r.first == next(m.begin(), 8));
			UT_ASSERT(r.second == next(m.begin(), 8));
			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->s);
			});
		}
		{
			typedef std::pair<M::const_iterator, M::const_iterator>
				R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			pmem::obj::transaction::run(pop, [&] {
				robj->s = nvobj::make_persistent<C>(
					ar, ar + sizeof(ar) / sizeof(ar[0]));
			});
			const auto &m = *robj->s;
			R r = m.equal_range(5);
			UT_ASSERT(r.first == next(m.begin(), 0));
			UT_ASSERT(r.second == next(m.begin(), 1));
			r = m.equal_range(7);
			UT_ASSERT(r.first == next(m.begin(), 1));
			UT_ASSERT(r.second == next(m.begin(), 2));
			r = m.equal_range(9);
			UT_ASSERT(r.first == next(m.begin(), 2));
			UT_ASSERT(r.second == next(m.begin(), 3));
			r = m.equal_range(11);
			UT_ASSERT(r.first == next(m.begin(), 3));
			UT_ASSERT(r.second == next(m.begin(), 4));
			r = m.equal_range(13);
			UT_ASSERT(r.first == next(m.begin(), 4));
			UT_ASSERT(r.second == next(m.begin(), 5));
			r = m.equal_range(15);
			UT_ASSERT(r.first == next(m.begin(), 5));
			UT_ASSERT(r.second == next(m.begin(), 6));
			r = m.equal_range(17);
			UT_ASSERT(r.first == next(m.begin(), 6));
			UT_ASSERT(r.second == next(m.begin(), 7));
			r = m.equal_range(19);
			UT_ASSERT(r.first == next(m.begin(), 7));
			UT_ASSERT(r.second == next(m.begin(), 8));
			r = m.equal_range(4);
			UT_ASSERT(r.first == next(m.begin(), 0));
			UT_ASSERT(r.second == next(m.begin(), 0));
			r = m.equal_range(6);
			UT_ASSERT(r.first == next(m.begin(), 1));
			UT_ASSERT(r.second == next(m.begin(), 1));
			r = m.equal_range(8);
			UT_ASSERT(r.first == next(m.begin(), 2));
			UT_ASSERT(r.second == next(m.begin(), 2));
			r = m.equal_range(10);
			UT_ASSERT(r.first == next(m.begin(), 3));
			UT_ASSERT(r.second == next(m.begin(), 3));
			r = m.equal_range(12);
			UT_ASSERT(r.first == next(m.begin(), 4));
			UT_ASSERT(r.second == next(m.begin(), 4));
			r = m.equal_range(14);
			UT_ASSERT(r.first == next(m.begin(), 5));
			UT_ASSERT(r.second == next(m.begin(), 5));
			r = m.equal_range(16);
			UT_ASSERT(r.first == next(m.begin(), 6));
			UT_ASSERT(r.second == next(m.begin(), 6));
			r = m.equal_range(18);
			UT_ASSERT(r.first == next(m.begin(), 7));
			UT_ASSERT(r.second == next(m.begin(), 7));
			r = m.equal_range(20);
			UT_ASSERT(r.first == next(m.begin(), 8));
			UT_ASSERT(r.second == next(m.begin(), 8));
			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->s);
			});
		}
	}
#ifdef XXX
	{
		typedef std::pair<const int, double> V;
		typedef std::map<int, double, std::less<int>, min_allocator<V>>
			M; // XXX: Port min_allocator
		{
			typedef std::pair<M::iterator, M::iterator> R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
			R r = m.equal_range(5);
			UT_ASSERT(r.first == next(m.begin(), 0));
			UT_ASSERT(r.second == next(m.begin(), 1));
			r = m.equal_range(7);
			UT_ASSERT(r.first == next(m.begin(), 1));
			UT_ASSERT(r.second == next(m.begin(), 2));
			r = m.equal_range(9);
			UT_ASSERT(r.first == next(m.begin(), 2));
			UT_ASSERT(r.second == next(m.begin(), 3));
			r = m.equal_range(11);
			UT_ASSERT(r.first == next(m.begin(), 3));
			UT_ASSERT(r.second == next(m.begin(), 4));
			r = m.equal_range(13);
			UT_ASSERT(r.first == next(m.begin(), 4));
			UT_ASSERT(r.second == next(m.begin(), 5));
			r = m.equal_range(15);
			UT_ASSERT(r.first == next(m.begin(), 5));
			UT_ASSERT(r.second == next(m.begin(), 6));
			r = m.equal_range(17);
			UT_ASSERT(r.first == next(m.begin(), 6));
			UT_ASSERT(r.second == next(m.begin(), 7));
			r = m.equal_range(19);
			UT_ASSERT(r.first == next(m.begin(), 7));
			UT_ASSERT(r.second == next(m.begin(), 8));
			r = m.equal_range(4);
			UT_ASSERT(r.first == next(m.begin(), 0));
			UT_ASSERT(r.second == next(m.begin(), 0));
			r = m.equal_range(6);
			UT_ASSERT(r.first == next(m.begin(), 1));
			UT_ASSERT(r.second == next(m.begin(), 1));
			r = m.equal_range(8);
			UT_ASSERT(r.first == next(m.begin(), 2));
			UT_ASSERT(r.second == next(m.begin(), 2));
			r = m.equal_range(10);
			UT_ASSERT(r.first == next(m.begin(), 3));
			UT_ASSERT(r.second == next(m.begin(), 3));
			r = m.equal_range(12);
			UT_ASSERT(r.first == next(m.begin(), 4));
			UT_ASSERT(r.second == next(m.begin(), 4));
			r = m.equal_range(14);
			UT_ASSERT(r.first == next(m.begin(), 5));
			UT_ASSERT(r.second == next(m.begin(), 5));
			r = m.equal_range(16);
			UT_ASSERT(r.first == next(m.begin(), 6));
			UT_ASSERT(r.second == next(m.begin(), 6));
			r = m.equal_range(18);
			UT_ASSERT(r.first == next(m.begin(), 7));
			UT_ASSERT(r.second == next(m.begin(), 7));
			r = m.equal_range(20);
			UT_ASSERT(r.first == next(m.begin(), 8));
			UT_ASSERT(r.second == next(m.begin(), 8));
		}
		{
			typedef std::pair<M::const_iterator, M::const_iterator>
				R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			const M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
			R r = m.equal_range(5);
			UT_ASSERT(r.first == next(m.begin(), 0));
			UT_ASSERT(r.second == next(m.begin(), 1));
			r = m.equal_range(7);
			UT_ASSERT(r.first == next(m.begin(), 1));
			UT_ASSERT(r.second == next(m.begin(), 2));
			r = m.equal_range(9);
			UT_ASSERT(r.first == next(m.begin(), 2));
			UT_ASSERT(r.second == next(m.begin(), 3));
			r = m.equal_range(11);
			UT_ASSERT(r.first == next(m.begin(), 3));
			UT_ASSERT(r.second == next(m.begin(), 4));
			r = m.equal_range(13);
			UT_ASSERT(r.first == next(m.begin(), 4));
			UT_ASSERT(r.second == next(m.begin(), 5));
			r = m.equal_range(15);
			UT_ASSERT(r.first == next(m.begin(), 5));
			UT_ASSERT(r.second == next(m.begin(), 6));
			r = m.equal_range(17);
			UT_ASSERT(r.first == next(m.begin(), 6));
			UT_ASSERT(r.second == next(m.begin(), 7));
			r = m.equal_range(19);
			UT_ASSERT(r.first == next(m.begin(), 7));
			UT_ASSERT(r.second == next(m.begin(), 8));
			r = m.equal_range(4);
			UT_ASSERT(r.first == next(m.begin(), 0));
			UT_ASSERT(r.second == next(m.begin(), 0));
			r = m.equal_range(6);
			UT_ASSERT(r.first == next(m.begin(), 1));
			UT_ASSERT(r.second == next(m.begin(), 1));
			r = m.equal_range(8);
			UT_ASSERT(r.first == next(m.begin(), 2));
			UT_ASSERT(r.second == next(m.begin(), 2));
			r = m.equal_range(10);
			UT_ASSERT(r.first == next(m.begin(), 3));
			UT_ASSERT(r.second == next(m.begin(), 3));
			r = m.equal_range(12);
			UT_ASSERT(r.first == next(m.begin(), 4));
			UT_ASSERT(r.second == next(m.begin(), 4));
			r = m.equal_range(14);
			UT_ASSERT(r.first == next(m.begin(), 5));
			UT_ASSERT(r.second == next(m.begin(), 5));
			r = m.equal_range(16);
			UT_ASSERT(r.first == next(m.begin(), 6));
			UT_ASSERT(r.second == next(m.begin(), 6));
			r = m.equal_range(18);
			UT_ASSERT(r.first == next(m.begin(), 7));
			UT_ASSERT(r.second == next(m.begin(), 7));
			r = m.equal_range(20);
			UT_ASSERT(r.first == next(m.begin(), 8));
			UT_ASSERT(r.second == next(m.begin(), 8));
		}
	}
#endif
	{
		auto robj = pop.root();
		typedef std::pair<const int, double> V;
		typedef C1 M;
		typedef std::pair<M::iterator, M::iterator> R;

		V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
			  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
		pmem::obj::transaction::run(pop, [&] {
			robj->s1 = nvobj::make_persistent<C1>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});
		auto &m = *robj->s1;
		R r = m.equal_range(5);
		UT_ASSERT(r.first == next(m.begin(), 0));
		UT_ASSERT(r.second == next(m.begin(), 1));
		r = m.equal_range(7);
		UT_ASSERT(r.first == next(m.begin(), 1));
		UT_ASSERT(r.second == next(m.begin(), 2));
		r = m.equal_range(9);
		UT_ASSERT(r.first == next(m.begin(), 2));
		UT_ASSERT(r.second == next(m.begin(), 3));
		r = m.equal_range(11);
		UT_ASSERT(r.first == next(m.begin(), 3));
		UT_ASSERT(r.second == next(m.begin(), 4));
		r = m.equal_range(13);
		UT_ASSERT(r.first == next(m.begin(), 4));
		UT_ASSERT(r.second == next(m.begin(), 5));
		r = m.equal_range(15);
		UT_ASSERT(r.first == next(m.begin(), 5));
		UT_ASSERT(r.second == next(m.begin(), 6));
		r = m.equal_range(17);
		UT_ASSERT(r.first == next(m.begin(), 6));
		UT_ASSERT(r.second == next(m.begin(), 7));
		r = m.equal_range(19);
		UT_ASSERT(r.first == next(m.begin(), 7));
		UT_ASSERT(r.second == next(m.begin(), 8));
		r = m.equal_range(4);
		UT_ASSERT(r.first == next(m.begin(), 0));
		UT_ASSERT(r.second == next(m.begin(), 0));
		r = m.equal_range(6);
		UT_ASSERT(r.first == next(m.begin(), 1));
		UT_ASSERT(r.second == next(m.begin(), 1));
		r = m.equal_range(8);
		UT_ASSERT(r.first == next(m.begin(), 2));
		UT_ASSERT(r.second == next(m.begin(), 2));
		r = m.equal_range(10);
		UT_ASSERT(r.first == next(m.begin(), 3));
		UT_ASSERT(r.second == next(m.begin(), 3));
		r = m.equal_range(12);
		UT_ASSERT(r.first == next(m.begin(), 4));
		UT_ASSERT(r.second == next(m.begin(), 4));
		r = m.equal_range(14);
		UT_ASSERT(r.first == next(m.begin(), 5));
		UT_ASSERT(r.second == next(m.begin(), 5));
		r = m.equal_range(16);
		UT_ASSERT(r.first == next(m.begin(), 6));
		UT_ASSERT(r.second == next(m.begin(), 6));
		r = m.equal_range(18);
		UT_ASSERT(r.first == next(m.begin(), 7));
		UT_ASSERT(r.second == next(m.begin(), 7));
		r = m.equal_range(20);
		UT_ASSERT(r.first == next(m.begin(), 8));
		UT_ASSERT(r.second == next(m.begin(), 8));

		r = m.equal_range(C2Int(5));
		UT_ASSERT(r.first == next(m.begin(), 0));
		UT_ASSERT(r.second == next(m.begin(), 1));
		r = m.equal_range(C2Int(7));
		UT_ASSERT(r.first == next(m.begin(), 1));
		UT_ASSERT(r.second == next(m.begin(), 2));
		r = m.equal_range(C2Int(9));
		UT_ASSERT(r.first == next(m.begin(), 2));
		UT_ASSERT(r.second == next(m.begin(), 3));
		r = m.equal_range(C2Int(11));
		UT_ASSERT(r.first == next(m.begin(), 3));
		UT_ASSERT(r.second == next(m.begin(), 4));
		r = m.equal_range(C2Int(13));
		UT_ASSERT(r.first == next(m.begin(), 4));
		UT_ASSERT(r.second == next(m.begin(), 5));
		r = m.equal_range(C2Int(15));
		UT_ASSERT(r.first == next(m.begin(), 5));
		UT_ASSERT(r.second == next(m.begin(), 6));
		r = m.equal_range(C2Int(17));
		UT_ASSERT(r.first == next(m.begin(), 6));
		UT_ASSERT(r.second == next(m.begin(), 7));
		r = m.equal_range(C2Int(19));
		UT_ASSERT(r.first == next(m.begin(), 7));
		UT_ASSERT(r.second == next(m.begin(), 8));
		r = m.equal_range(C2Int(4));
		UT_ASSERT(r.first == next(m.begin(), 0));
		UT_ASSERT(r.second == next(m.begin(), 0));
		r = m.equal_range(C2Int(6));
		UT_ASSERT(r.first == next(m.begin(), 1));
		UT_ASSERT(r.second == next(m.begin(), 1));
		r = m.equal_range(C2Int(8));
		UT_ASSERT(r.first == next(m.begin(), 2));
		UT_ASSERT(r.second == next(m.begin(), 2));
		r = m.equal_range(C2Int(10));
		UT_ASSERT(r.first == next(m.begin(), 3));
		UT_ASSERT(r.second == next(m.begin(), 3));
		r = m.equal_range(C2Int(12));
		UT_ASSERT(r.first == next(m.begin(), 4));
		UT_ASSERT(r.second == next(m.begin(), 4));
		r = m.equal_range(C2Int(14));
		UT_ASSERT(r.first == next(m.begin(), 5));
		UT_ASSERT(r.second == next(m.begin(), 5));
		r = m.equal_range(C2Int(16));
		UT_ASSERT(r.first == next(m.begin(), 6));
		UT_ASSERT(r.second == next(m.begin(), 6));
		r = m.equal_range(C2Int(18));
		UT_ASSERT(r.first == next(m.begin(), 7));
		UT_ASSERT(r.second == next(m.begin(), 7));
		r = m.equal_range(C2Int(20));
		UT_ASSERT(r.first == next(m.begin(), 8));
		UT_ASSERT(r.second == next(m.begin(), 8));
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s1); });
	}
	{
		auto robj = pop.root();
		typedef PrivateConstructor PC;
		typedef C2 M;
		typedef std::pair<M::iterator, M::iterator> R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<C2>(); });
		auto &m = *robj->s2;

		m.insert({PC::make(5), 5});
		m.insert({PC::make(7), 6});
		m.insert({PC::make(9), 7});
		m.insert({PC::make(11), 8});
		m.insert({PC::make(13), 9});
		m.insert({PC::make(15), 10});
		m.insert({PC::make(17), 11});
		m.insert({PC::make(19), 12});

		R r = m.equal_range(5);
		UT_ASSERT(r.first == next(m.begin(), 0));
		UT_ASSERT(r.second == next(m.begin(), 1));
		r = m.equal_range(7);
		UT_ASSERT(r.first == next(m.begin(), 1));
		UT_ASSERT(r.second == next(m.begin(), 2));
		r = m.equal_range(9);
		UT_ASSERT(r.first == next(m.begin(), 2));
		UT_ASSERT(r.second == next(m.begin(), 3));
		r = m.equal_range(11);
		UT_ASSERT(r.first == next(m.begin(), 3));
		UT_ASSERT(r.second == next(m.begin(), 4));
		r = m.equal_range(13);
		UT_ASSERT(r.first == next(m.begin(), 4));
		UT_ASSERT(r.second == next(m.begin(), 5));
		r = m.equal_range(15);
		UT_ASSERT(r.first == next(m.begin(), 5));
		UT_ASSERT(r.second == next(m.begin(), 6));
		r = m.equal_range(17);
		UT_ASSERT(r.first == next(m.begin(), 6));
		UT_ASSERT(r.second == next(m.begin(), 7));
		r = m.equal_range(19);
		UT_ASSERT(r.first == next(m.begin(), 7));
		UT_ASSERT(r.second == next(m.begin(), 8));
		r = m.equal_range(4);
		UT_ASSERT(r.first == next(m.begin(), 0));
		UT_ASSERT(r.second == next(m.begin(), 0));
		r = m.equal_range(6);
		UT_ASSERT(r.first == next(m.begin(), 1));
		UT_ASSERT(r.second == next(m.begin(), 1));
		r = m.equal_range(8);
		UT_ASSERT(r.first == next(m.begin(), 2));
		UT_ASSERT(r.second == next(m.begin(), 2));
		r = m.equal_range(10);
		UT_ASSERT(r.first == next(m.begin(), 3));
		UT_ASSERT(r.second == next(m.begin(), 3));
		r = m.equal_range(12);
		UT_ASSERT(r.first == next(m.begin(), 4));
		UT_ASSERT(r.second == next(m.begin(), 4));
		r = m.equal_range(14);
		UT_ASSERT(r.first == next(m.begin(), 5));
		UT_ASSERT(r.second == next(m.begin(), 5));
		r = m.equal_range(16);
		UT_ASSERT(r.first == next(m.begin(), 6));
		UT_ASSERT(r.second == next(m.begin(), 6));
		r = m.equal_range(18);
		UT_ASSERT(r.first == next(m.begin(), 7));
		UT_ASSERT(r.second == next(m.begin(), 7));
		r = m.equal_range(20);
		UT_ASSERT(r.first == next(m.begin(), 8));
		UT_ASSERT(r.second == next(m.begin(), 8));
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
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
		pop = pmem::obj::pool<root>::create(path, "equal_range.pass",
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
