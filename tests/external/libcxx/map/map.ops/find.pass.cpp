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

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../is_transparent.h"
#include "../private_constructor.h"

/* Windows has a max macro which collides with std::numeric_limits::min */
#if defined(min) && defined(_WIN32)
#undef min
#endif
#if defined(max) && defined(_WIN32)
#undef max
#endif

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = container_t<int, double>;
using C1 = container_t<int, double, TRANSPARENT_COMPARE>;
using C2 = container_t<PrivateConstructor, double, TRANSPARENT_COMPARE>;
using C3 = container_t<uint64_t, uint64_t>;

struct root {
	nvobj::persistent_ptr<C> s;
	nvobj::persistent_ptr<C1> s1;
	nvobj::persistent_ptr<C2> s2;
	nvobj::persistent_ptr<C3> s3;
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
				robj->s = nvobj::make_persistent<M>(
					ar, ar + sizeof(ar) / sizeof(ar[0]));
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
				robj->s = nvobj::make_persistent<M>(
					ar, ar + sizeof(ar) / sizeof(ar[0]));
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
		{
			typedef M::iterator R;

			V ar[] = {V(-5, -5),
				  V(-6, -6),
				  V(-7, -7),
				  V(std::numeric_limits<int>::min(),
				    std::numeric_limits<int>::min()),
				  V(9, 9),
				  V(10, 10),
				  V(11, 11),
				  V(12, 12),
				  V(std::numeric_limits<int>::max(),
				    std::numeric_limits<int>::max())};
			pmem::obj::transaction::run(pop, [&] {
				robj->s = nvobj::make_persistent<M>(
					ar, ar + sizeof(ar) / sizeof(ar[0]));
			});
			auto &m = *robj->s;
			R r = m.find(std::numeric_limits<int>::min());
			UT_ASSERT(r == m.begin());
			r = m.find(-7);
			UT_ASSERT(r == std::next(m.begin()));
			r = m.find(-6);
			UT_ASSERT(r == std::next(m.begin(), 2));
			r = m.find(-5);
			UT_ASSERT(r == std::next(m.begin(), 3));
			r = m.find(9);
			UT_ASSERT(r == std::next(m.begin(), 4));
			r = m.find(10);
			UT_ASSERT(r == std::next(m.begin(), 5));
			r = m.find(11);
			UT_ASSERT(r == std::next(m.begin(), 6));
			r = m.find(12);
			UT_ASSERT(r == std::next(m.begin(), 7));
			r = m.find(std::numeric_limits<int>::max());
			UT_ASSERT(r == std::next(m.begin(), 8));
			r = m.find(4);
			UT_ASSERT(r == std::next(m.begin(), 9));
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
			robj->s1 = nvobj::make_persistent<M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
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

		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s1); });
	}
	{
		typedef PrivateConstructor PC;
		typedef C2 M;
		typedef M::iterator R;

		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<C2>(); });
		auto &m = *robj->s2;
		for (int i = 5; i <= 12; i++) {
			m.emplace(PC::make(i), i);
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
	{
		typedef C3 M;
		typedef M::value_type V;
		typedef M::iterator R;

		auto half = std::numeric_limits<uint64_t>::max() / 2;

		V ar[] = {V(half - 5, half - 5),   V(half - 6, half - 6),
			  V(half - 7, half - 7),   V(half - 8, half - 8),
			  V(half + 9, half + 9),   V(half + 10, half + 10),
			  V(half + 11, half + 11), V(half + 12, half + 12)};
		pmem::obj::transaction::run(pop, [&] {
			robj->s3 = nvobj::make_persistent<M>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
		});
		auto &m = *robj->s3;
		R r = m.find(half - 8);
		UT_ASSERT(r == m.begin());
		r = m.find(half - 7);
		UT_ASSERT(r == std::next(m.begin()));
		r = m.find(half - 6);
		UT_ASSERT(r == std::next(m.begin(), 2));
		r = m.find(half - 5);
		UT_ASSERT(r == std::next(m.begin(), 3));
		r = m.find(half + 9);
		UT_ASSERT(r == std::next(m.begin(), 4));
		r = m.find(half + 10);
		UT_ASSERT(r == std::next(m.begin(), 5));
		r = m.find(half + 11);
		UT_ASSERT(r == std::next(m.begin(), 6));
		r = m.find(half + 12);
		UT_ASSERT(r == std::next(m.begin(), 7));
		r = m.find(4);
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
