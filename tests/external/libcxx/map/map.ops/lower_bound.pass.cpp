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

//       iterator lower_bound(const key_type& k);
// const_iterator lower_bound(const key_type& k) const;

#include "unittest.hpp"

#include "map_wrapper.hpp"

#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../is_transparent.h"
#include "../private_constructor.h"

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = container_t<int, double>;
using C2 = container_t<int, double, TRANSPARENT_COMPARE>;
using C3 = container_t<PrivateConstructor, double, TRANSPARENT_COMPARE>;
struct root {
	nvobj::persistent_ptr<C> s;
	nvobj::persistent_ptr<C2> s2;
	nvobj::persistent_ptr<C3> s3;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef std::pair<const int, double> V;
		typedef C M;
		{
			typedef M::iterator R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			pmem::obj::transaction::run(pop, [&] {
				robj->s = nvobj::make_persistent<M>(
					ar, ar + sizeof(ar) / sizeof(ar[0]));
			});
			M &m = *robj->s;
			R r = m.lower_bound(5);
			UT_ASSERT(r == m.begin());
			r = m.lower_bound(7);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.lower_bound(9);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.lower_bound(11);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.lower_bound(13);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.lower_bound(15);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.lower_bound(17);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.lower_bound(19);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.lower_bound(4);
			UT_ASSERT(r == std::next(m.begin(), 0));
			r = m.lower_bound(6);
			UT_ASSERT(r == std::next(m.begin(), 1));
			r = m.lower_bound(8);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.lower_bound(10);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.lower_bound(12);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.lower_bound(14);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.lower_bound(16);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.lower_bound(18);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.lower_bound(20);
			UT_ASSERT(r == std::next(m.begin(), 8));
			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->s);
			});
		}
		{
			typedef M::const_iterator R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			pmem::obj::transaction::run(pop, [&] {
				robj->s = nvobj::make_persistent<M>(
					ar, ar + sizeof(ar) / sizeof(ar[0]));
			});
			const M &m = *robj->s;
			R r = m.lower_bound(5);
			UT_ASSERT(r == m.begin());
			r = m.lower_bound(7);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.lower_bound(9);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.lower_bound(11);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.lower_bound(13);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.lower_bound(15);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.lower_bound(17);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.lower_bound(19);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.lower_bound(4);
			UT_ASSERT(r == std::next(m.begin(), 0));
			r = m.lower_bound(6);
			UT_ASSERT(r == std::next(m.begin(), 1));
			r = m.lower_bound(8);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.lower_bound(10);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.lower_bound(12);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.lower_bound(14);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.lower_bound(16);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.lower_bound(18);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.lower_bound(20);
			UT_ASSERT(r == std::next(m.begin(), 8));
			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->s);
			});
		}
	}
#ifdef XXX
	{ // XXX: Implement min_allocator class
		typedef std::pair<const int, double> V;
		typedef std::map<int, double, std::less<int>, min_allocator<V>>
			M;
		{
			typedef M::iterator R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
			R r = m.lower_bound(5);
			UT_ASSERT(r == m.begin());
			r = m.lower_bound(7);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.lower_bound(9);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.lower_bound(11);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.lower_bound(13);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.lower_bound(15);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.lower_bound(17);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.lower_bound(19);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.lower_bound(4);
			UT_ASSERT(r == std::next(m.begin(), 0));
			r = m.lower_bound(6);
			UT_ASSERT(r == std::next(m.begin(), 1));
			r = m.lower_bound(8);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.lower_bound(10);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.lower_bound(12);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.lower_bound(14);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.lower_bound(16);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.lower_bound(18);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.lower_bound(20);
			UT_ASSERT(r == std::next(m.begin(), 8));
		}
		{
			typedef M::const_iterator R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			const M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
			R r = m.lower_bound(5);
			UT_ASSERT(r == m.begin());
			r = m.lower_bound(7);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.lower_bound(9);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.lower_bound(11);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.lower_bound(13);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.lower_bound(15);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.lower_bound(17);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.lower_bound(19);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.lower_bound(4);
			UT_ASSERT(r == std::next(m.begin(), 0));
			r = m.lower_bound(6);
			UT_ASSERT(r == std::next(m.begin(), 1));
			r = m.lower_bound(8);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.lower_bound(10);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.lower_bound(12);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.lower_bound(14);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.lower_bound(16);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.lower_bound(18);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.lower_bound(20);
			UT_ASSERT(r == std::next(m.begin(), 8));
		}
	}
#endif
	{
		typedef std::pair<const int, double> V;
		typedef C2 M;
		typedef M::iterator R;

		V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
			  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
		pmem::obj::transaction::run(pop, [&] {
			robj->s2 = nvobj::make_persistent<M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});
		M &m = *robj->s2;
		R r = m.lower_bound(5);
		UT_ASSERT(r == m.begin());
		r = m.lower_bound(7);
		UT_ASSERT(r == std::next(m.begin()));
		r = m.lower_bound(9);
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.lower_bound(11);
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.lower_bound(13);
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.lower_bound(15);
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.lower_bound(17);
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.lower_bound(19);
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.lower_bound(4);
		UT_ASSERT(r == std::next(m.begin(), 0));
		r = m.lower_bound(6);
		UT_ASSERT(r == std::next(m.begin(), 1));
		r = m.lower_bound(8);
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.lower_bound(10);
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.lower_bound(12);
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.lower_bound(14);
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.lower_bound(16);
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.lower_bound(18);
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.lower_bound(20);
		UT_ASSERT(r == std::next(m.begin(), 8));

		r = m.lower_bound(C2Int(5));
		UT_ASSERT(r == m.begin());
		r = m.lower_bound(C2Int(7));
		UT_ASSERT(r == std::next(m.begin()));
		r = m.lower_bound(C2Int(9));
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.lower_bound(C2Int(11));
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.lower_bound(C2Int(13));
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.lower_bound(C2Int(15));
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.lower_bound(C2Int(17));
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.lower_bound(C2Int(19));
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.lower_bound(C2Int(4));
		UT_ASSERT(r == std::next(m.begin(), 0));
		r = m.lower_bound(C2Int(6));
		UT_ASSERT(r == std::next(m.begin(), 1));
		r = m.lower_bound(C2Int(8));
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.lower_bound(C2Int(10));
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.lower_bound(C2Int(12));
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.lower_bound(C2Int(14));
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.lower_bound(C2Int(16));
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.lower_bound(C2Int(18));
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.lower_bound(C2Int(20));
		UT_ASSERT(r == std::next(m.begin(), 8));
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
	{
		typedef PrivateConstructor PC;
		typedef C3 M;
		typedef M::iterator R;
		pmem::obj::transaction::run(
			pop, [&] { robj->s3 = nvobj::make_persistent<M>(); });
		M &m = *robj->s3;
		m.insert({PC::make(5), 5});
		m.insert({PC::make(7), 6});
		m.insert({PC::make(9), 7});
		m.insert({PC::make(11), 8});
		m.insert({PC::make(13), 9});
		m.insert({PC::make(15), 10});
		m.insert({PC::make(17), 11});
		m.insert({PC::make(19), 12});

		R r = m.lower_bound(5);
		UT_ASSERT(r == m.begin());
		r = m.lower_bound(7);
		UT_ASSERT(r == std::next(m.begin()));
		r = m.lower_bound(9);
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.lower_bound(11);
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.lower_bound(13);
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.lower_bound(15);
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.lower_bound(17);
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.lower_bound(19);
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.lower_bound(4);
		UT_ASSERT(r == std::next(m.begin(), 0));
		r = m.lower_bound(6);
		UT_ASSERT(r == std::next(m.begin(), 1));
		r = m.lower_bound(8);
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.lower_bound(10);
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.lower_bound(12);
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.lower_bound(14);
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.lower_bound(16);
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.lower_bound(18);
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.lower_bound(20);
		UT_ASSERT(r == std::next(m.begin(), 8));
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s3); });
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
		pop = pmem::obj::pool<root>::create(path, "lower_bound.pass",
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
