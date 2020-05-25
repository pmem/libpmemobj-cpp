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

// size_type count(const key_type& k) const;

#include "../is_transparent.h"
#include "../private_constructor.h"
#include "unittest.hpp"

#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = nvobjex::concurrent_map<int, double>;
#ifdef XXX // Implement generic std::less
using C2 = nvobjex::concurrent_map<int, double, std::less<>>;
using C3 = nvobjex::concurrent_map<PrivateConstructor, double, std::less<>>;
#endif
struct root {
	nvobj::persistent_ptr<C> s;
#ifdef XXX // Implement generic std::less
	nvobj::persistent_ptr<C2> s2;
	nvobj::persistent_ptr<C3> s3;
#endif
};

int
run(pmem::obj::pool<root> &pop)
{
	{
		auto robj = pop.root();

		typedef std::pair<const int, double> V;
		typedef C M;
		{
			typedef M::size_type R;
			V ar[] = {V(5, 5), V(6, 6),   V(7, 7),	 V(8, 8),
				  V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
			pmem::obj::transaction::run(pop, [&] {
				robj->s = nvobj::make_persistent<C>(
					ar, ar + sizeof(ar) / sizeof(ar[0]));
			});
			const auto &m = *robj->s;
			R r = m.count(5);
			UT_ASSERTeq(r, 1);
			r = m.count(6);
			UT_ASSERTeq(r, 1);
			r = m.count(7);
			UT_ASSERTeq(r, 1);
			r = m.count(8);
			UT_ASSERTeq(r, 1);
			r = m.count(9);
			UT_ASSERTeq(r, 1);
			r = m.count(10);
			UT_ASSERTeq(r, 1);
			r = m.count(11);
			UT_ASSERTeq(r, 1);
			r = m.count(12);
			UT_ASSERTeq(r, 1);
			r = m.count(4);
			UT_ASSERTeq(r, 0);
			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<C>(robj->s);
			});
		}
	}
#ifdef XXX
	{
		auto r = pop.root();
		typedef std::pair<const int, double> V;
		// XXX(kfilipek): implement min_allocator class
		typedef nvobjex::concurrent_map<int, double, std::less<int>,
						min_allocator<V>>
			M;

		{
			typedef M::size_type R;
			V ar[] = {V(5, 5), V(6, 6),   V(7, 7),	 V(8, 8),
				  V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
			r->s = nvobj::make_persistent<C>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
			const auto m = *r->s;

			R r = m.count(5);
			UT_ASSERT(r == 1);
			r = m.count(6);
			UT_ASSERT(r == 1);
			r = m.count(7);
			UT_ASSERT(r == 1);
			r = m.count(8);
			UT_ASSERT(r == 1);
			r = m.count(9);
			UT_ASSERT(r == 1);
			r = m.count(10);
			UT_ASSERT(r == 1);
			r = m.count(11);
			UT_ASSERT(r == 1);
			r = m.count(12);
			UT_ASSERT(r == 1);
			r = m.count(4);
			UT_ASSERT(r == 0);
		}
	}
#endif
#ifdef XXX // Implement generic std::less
	{
		auto robj = pop.root();

		typedef std::pair<const int, double> V;
		typedef C2 M;
		typedef M::size_type R;

		V ar[] = {V(5, 5), V(6, 6),   V(7, 7),	 V(8, 8),
			  V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
		pmem::obj::transaction::run(pop, [&] {
			robj->s2 = nvobj::make_persistent<C2>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});
		const auto &m = *robj->s2;
		R r = m.count(5);
		UT_ASSERTeq(r, 1);
		r = m.count(6);
		UT_ASSERTeq(r, 1);
		r = m.count(7);
		UT_ASSERTeq(r, 1);
		r = m.count(8);
		UT_ASSERTeq(r, 1);
		r = m.count(9);
		UT_ASSERTeq(r, 1);
		r = m.count(10);
		UT_ASSERTeq(r, 1);
		r = m.count(11);
		UT_ASSERTeq(r, 1);
		r = m.count(12);
		UT_ASSERTeq(r, 1);
		r = m.count(4);
		UT_ASSERTeq(r, 0);

		r = m.count(C2Int(5));
		UT_ASSERTeq(r, 1);
		r = m.count(C2Int(6));
		UT_ASSERTeq(r, 1);
		r = m.count(C2Int(7));
		UT_ASSERTeq(r, 1);
		r = m.count(C2Int(8));
		UT_ASSERTeq(r, 1);
		r = m.count(C2Int(9));
		UT_ASSERTeq(r, 1);
		r = m.count(C2Int(10));
		UT_ASSERTeq(r, 1);
		r = m.count(C2Int(11));
		UT_ASSERTeq(r, 1);
		r = m.count(C2Int(12));
		UT_ASSERTeq(r, 1);
		r = m.count(C2Int(4));
		UT_ASSERTeq(r, 0);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C2>(robj->s2); });
	}
	{
		auto robj = pop.root();

		typedef PrivateConstructor PC;
		typedef C3 M;
		typedef M::size_type R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s3 = nvobj::make_persistent<C3>(); });
		auto &m = *robj->s3;

		m.insert({PC::make(5), 5});
		m.insert({PC::make(6), 6});
		m.insert({PC::make(7), 7});
		m.insert({PC::make(8), 8});
		m.insert({PC::make(9), 9});
		m.insert({PC::make(10), 10});
		m.insert({PC::make(11), 11});
		m.insert({PC::make(12), 12});

		R r = m.count(5);
		UT_ASSERTeq(r, 1);
		r = m.count(6);
		UT_ASSERTeq(r, 1);
		r = m.count(7);
		UT_ASSERTeq(r, 1);
		r = m.count(8);
		UT_ASSERTeq(r, 1);
		r = m.count(9);
		UT_ASSERTeq(r, 1);
		r = m.count(10);
		UT_ASSERTeq(r, 1);
		r = m.count(11);
		UT_ASSERTeq(r, 1);
		r = m.count(12);
		UT_ASSERTeq(r, 1);
		r = m.count(4);
		UT_ASSERTeq(r, 0);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C3>(robj->s3); });
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
		pop = pmem::obj::pool<root>::create(path, "count.pass",
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
