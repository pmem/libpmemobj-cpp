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
//

// <map>

// class map

//       iterator find(const key_type& k);
// const_iterator find(const key_type& k) const;

#include "map_wrapper.hpp"
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

using C = container_t<int, double>;
using C1 = container_t<int, double, transparent_less>;
using C2 = container_t<PrivateConstructor, double, transparent_less>;
struct root {
	nvobj::persistent_ptr<C> s;
	nvobj::persistent_ptr<C1> s1;
	nvobj::persistent_ptr<C2> s2;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef C M;
		typedef M::value_type V;
		{
			typedef M::iterator R;
			V ar[] = {V(5, 5), V(6, 6),   V(7, 7),	 V(8, 8),
				  V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
			pmem::obj::transaction::run(pop, [&] {
				robj->s = nvobj::make_persistent<M>();
				for (auto &e : ar)
					robj->s->emplace(e);
			});
			auto &m = *robj->s;
			R r = m.find(5);
			UT_ASSERT(r == m.begin());
			r = m.find(6);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.find(7);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.find(8);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.find(9);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.find(10);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.find(11);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.find(12);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.find(4);
			UT_ASSERT(r == std::next(m.begin(), 8));
			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->s);
			});
		}
		{
			typedef M::const_iterator R;
			V ar[] = {V(5, 5), V(6, 6),   V(7, 7),	 V(8, 8),
				  V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
			pmem::obj::transaction::run(pop, [&] {
				for (auto &e : ar)
					robj->s->emplace(e);
			});
			const auto &m = *robj->s;
			R r = m.find(5);
			UT_ASSERT(r == m.begin());
			r = m.find(6);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.find(7);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.find(8);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.find(9);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.find(10);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.find(11);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.find(12);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.find(4);
			UT_ASSERT(r == std::next(m.begin(), 8));
			pmem::obj::transaction::run(pop, [&] {
				nvobj::delete_persistent<M>(robj->s);
			});
		}
	}
#ifdef XXX // XXX: Implement min_allocator
	{
		typedef std::pair<const int, double> V;
		typedef std::map<int, double, std::less<int>, min_allocator<V>>
			M;
		{
			typedef M::iterator R;
			V ar[] = {V(5, 5), V(6, 6),   V(7, 7),	 V(8, 8),
				  V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
			M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
			R r = m.find(5);
			UT_ASSERT(r == m.begin());
			r = m.find(6);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.find(7);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.find(8);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.find(9);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.find(10);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.find(11);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.find(12);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.find(4);
			UT_ASSERT(r == std::next(m.begin(), 8));
		}
		{
			typedef M::const_iterator R;
			V ar[] = {V(5, 5), V(6, 6),   V(7, 7),	 V(8, 8),
				  V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
			const M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
			R r = m.find(5);
			UT_ASSERT(r == m.begin());
			r = m.find(6);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.find(7);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.find(8);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.find(9);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.find(10);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.find(11);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.find(12);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.find(4);
			UT_ASSERT(r == std::next(m.begin(), 8));
		}
	}
#endif
	{
		typedef C1 M;
		typedef M::value_type V;
		typedef M::iterator R;

		V ar[] = {V(5, 5), V(6, 6),   V(7, 7),	 V(8, 8),
			  V(9, 9), V(10, 10), V(11, 11), V(12, 12)};
		pmem::obj::transaction::run(pop, [&] {
			robj->s1 = nvobj::make_persistent<M>();
			for (auto &e : ar)
				robj->s1->emplace(e);
		});
		auto &m = *robj->s1;
		R r = m.find(5);
		UT_ASSERT(r == m.begin());
		r = m.find(6);
		UT_ASSERT(r == std::next(m.begin()));
		r = m.find(7);
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.find(8);
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.find(9);
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.find(10);
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.find(11);
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.find(12);
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.find(4);
		UT_ASSERT(r == std::next(m.begin(), 8));

#ifndef RADIX
		r = m.find(C2Int(5));
		UT_ASSERT(r == m.begin());
		r = m.find(C2Int(6));
		UT_ASSERT(r == std::next(m.begin()));
		r = m.find(C2Int(7));
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.find(C2Int(8));
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.find(C2Int(9));
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.find(C2Int(10));
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.find(C2Int(11));
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.find(C2Int(12));
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.find(C2Int(4));
		UT_ASSERT(r == std::next(m.begin(), 8));
#endif
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s1); });
	}

#ifndef RADIX
	{
		typedef PrivateConstructor PC;
		typedef C2 M;
		typedef M::iterator R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<C2>(); });
		auto &m = *robj->s2;
		for (int i = 5; i <= 12; i++) {
			m.insert({PC::make(i), i});
		}

		R r = m.find(5);
		UT_ASSERT(r == m.begin());
		r = m.find(6);
		UT_ASSERT(r == std::next(m.begin()));
		r = m.find(7);
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.find(8);
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.find(9);
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.find(10);
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.find(11);
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.find(12);
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.find(4);
		UT_ASSERT(r == std::next(m.begin(), 8));
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
		pop = pmem::obj::pool<root>::create(
			path, "find.pass", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
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
