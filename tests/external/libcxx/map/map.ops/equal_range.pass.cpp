//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

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

// #include "is_transparent.h"
// #include "min_allocator.h"
// #include "private_constructor.h"
// #include "test_macros.h"

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;
using C = nvobjex::concurrent_map<int, double>;

struct root {
	nvobj::persistent_ptr<C> s;
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
			robj->s = nvobj::make_persistent<C>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
			auto m = *robj->s;
			R r = m.equal_range(5);
			assert(r.first == next(m.begin(), 0));
			assert(r.second == next(m.begin(), 1));
			r = m.equal_range(7);
			assert(r.first == next(m.begin(), 1));
			assert(r.second == next(m.begin(), 2));
			r = m.equal_range(9);
			assert(r.first == next(m.begin(), 2));
			assert(r.second == next(m.begin(), 3));
			r = m.equal_range(11);
			assert(r.first == next(m.begin(), 3));
			assert(r.second == next(m.begin(), 4));
			r = m.equal_range(13);
			assert(r.first == next(m.begin(), 4));
			assert(r.second == next(m.begin(), 5));
			r = m.equal_range(15);
			assert(r.first == next(m.begin(), 5));
			assert(r.second == next(m.begin(), 6));
			r = m.equal_range(17);
			assert(r.first == next(m.begin(), 6));
			assert(r.second == next(m.begin(), 7));
			r = m.equal_range(19);
			assert(r.first == next(m.begin(), 7));
			assert(r.second == next(m.begin(), 8));
			r = m.equal_range(4);
			assert(r.first == next(m.begin(), 0));
			assert(r.second == next(m.begin(), 0));
			r = m.equal_range(6);
			assert(r.first == next(m.begin(), 1));
			assert(r.second == next(m.begin(), 1));
			r = m.equal_range(8);
			assert(r.first == next(m.begin(), 2));
			assert(r.second == next(m.begin(), 2));
			r = m.equal_range(10);
			assert(r.first == next(m.begin(), 3));
			assert(r.second == next(m.begin(), 3));
			r = m.equal_range(12);
			assert(r.first == next(m.begin(), 4));
			assert(r.second == next(m.begin(), 4));
			r = m.equal_range(14);
			assert(r.first == next(m.begin(), 5));
			assert(r.second == next(m.begin(), 5));
			r = m.equal_range(16);
			assert(r.first == next(m.begin(), 6));
			assert(r.second == next(m.begin(), 6));
			r = m.equal_range(18);
			assert(r.first == next(m.begin(), 7));
			assert(r.second == next(m.begin(), 7));
			r = m.equal_range(20);
			assert(r.first == next(m.begin(), 8));
			assert(r.second == next(m.begin(), 8));
		}
		{
			typedef std::pair<M::const_iterator, M::const_iterator>
				R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			// const M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
			robj->s = nvobj::make_persistent<C>(
				ar, ar + sizeof(ar) / sizeof(ar[0]));
			const auto m = *robj->s;
			R r = m.equal_range(5);
			assert(r.first == next(m.begin(), 0));
			assert(r.second == next(m.begin(), 1));
			r = m.equal_range(7);
			assert(r.first == next(m.begin(), 1));
			assert(r.second == next(m.begin(), 2));
			r = m.equal_range(9);
			assert(r.first == next(m.begin(), 2));
			assert(r.second == next(m.begin(), 3));
			r = m.equal_range(11);
			assert(r.first == next(m.begin(), 3));
			assert(r.second == next(m.begin(), 4));
			r = m.equal_range(13);
			assert(r.first == next(m.begin(), 4));
			assert(r.second == next(m.begin(), 5));
			r = m.equal_range(15);
			assert(r.first == next(m.begin(), 5));
			assert(r.second == next(m.begin(), 6));
			r = m.equal_range(17);
			assert(r.first == next(m.begin(), 6));
			assert(r.second == next(m.begin(), 7));
			r = m.equal_range(19);
			assert(r.first == next(m.begin(), 7));
			assert(r.second == next(m.begin(), 8));
			r = m.equal_range(4);
			assert(r.first == next(m.begin(), 0));
			assert(r.second == next(m.begin(), 0));
			r = m.equal_range(6);
			assert(r.first == next(m.begin(), 1));
			assert(r.second == next(m.begin(), 1));
			r = m.equal_range(8);
			assert(r.first == next(m.begin(), 2));
			assert(r.second == next(m.begin(), 2));
			r = m.equal_range(10);
			assert(r.first == next(m.begin(), 3));
			assert(r.second == next(m.begin(), 3));
			r = m.equal_range(12);
			assert(r.first == next(m.begin(), 4));
			assert(r.second == next(m.begin(), 4));
			r = m.equal_range(14);
			assert(r.first == next(m.begin(), 5));
			assert(r.second == next(m.begin(), 5));
			r = m.equal_range(16);
			assert(r.first == next(m.begin(), 6));
			assert(r.second == next(m.begin(), 6));
			r = m.equal_range(18);
			assert(r.first == next(m.begin(), 7));
			assert(r.second == next(m.begin(), 7));
			r = m.equal_range(20);
			assert(r.first == next(m.begin(), 8));
			assert(r.second == next(m.begin(), 8));
		}
	}
#if TEST_STD_VER >= 11
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
			assert(r.first == next(m.begin(), 0));
			assert(r.second == next(m.begin(), 1));
			r = m.equal_range(7);
			assert(r.first == next(m.begin(), 1));
			assert(r.second == next(m.begin(), 2));
			r = m.equal_range(9);
			assert(r.first == next(m.begin(), 2));
			assert(r.second == next(m.begin(), 3));
			r = m.equal_range(11);
			assert(r.first == next(m.begin(), 3));
			assert(r.second == next(m.begin(), 4));
			r = m.equal_range(13);
			assert(r.first == next(m.begin(), 4));
			assert(r.second == next(m.begin(), 5));
			r = m.equal_range(15);
			assert(r.first == next(m.begin(), 5));
			assert(r.second == next(m.begin(), 6));
			r = m.equal_range(17);
			assert(r.first == next(m.begin(), 6));
			assert(r.second == next(m.begin(), 7));
			r = m.equal_range(19);
			assert(r.first == next(m.begin(), 7));
			assert(r.second == next(m.begin(), 8));
			r = m.equal_range(4);
			assert(r.first == next(m.begin(), 0));
			assert(r.second == next(m.begin(), 0));
			r = m.equal_range(6);
			assert(r.first == next(m.begin(), 1));
			assert(r.second == next(m.begin(), 1));
			r = m.equal_range(8);
			assert(r.first == next(m.begin(), 2));
			assert(r.second == next(m.begin(), 2));
			r = m.equal_range(10);
			assert(r.first == next(m.begin(), 3));
			assert(r.second == next(m.begin(), 3));
			r = m.equal_range(12);
			assert(r.first == next(m.begin(), 4));
			assert(r.second == next(m.begin(), 4));
			r = m.equal_range(14);
			assert(r.first == next(m.begin(), 5));
			assert(r.second == next(m.begin(), 5));
			r = m.equal_range(16);
			assert(r.first == next(m.begin(), 6));
			assert(r.second == next(m.begin(), 6));
			r = m.equal_range(18);
			assert(r.first == next(m.begin(), 7));
			assert(r.second == next(m.begin(), 7));
			r = m.equal_range(20);
			assert(r.first == next(m.begin(), 8));
			assert(r.second == next(m.begin(), 8));
		}
		{
			typedef std::pair<M::const_iterator, M::const_iterator>
				R;
			V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
				  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
			const M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
			R r = m.equal_range(5);
			assert(r.first == next(m.begin(), 0));
			assert(r.second == next(m.begin(), 1));
			r = m.equal_range(7);
			assert(r.first == next(m.begin(), 1));
			assert(r.second == next(m.begin(), 2));
			r = m.equal_range(9);
			assert(r.first == next(m.begin(), 2));
			assert(r.second == next(m.begin(), 3));
			r = m.equal_range(11);
			assert(r.first == next(m.begin(), 3));
			assert(r.second == next(m.begin(), 4));
			r = m.equal_range(13);
			assert(r.first == next(m.begin(), 4));
			assert(r.second == next(m.begin(), 5));
			r = m.equal_range(15);
			assert(r.first == next(m.begin(), 5));
			assert(r.second == next(m.begin(), 6));
			r = m.equal_range(17);
			assert(r.first == next(m.begin(), 6));
			assert(r.second == next(m.begin(), 7));
			r = m.equal_range(19);
			assert(r.first == next(m.begin(), 7));
			assert(r.second == next(m.begin(), 8));
			r = m.equal_range(4);
			assert(r.first == next(m.begin(), 0));
			assert(r.second == next(m.begin(), 0));
			r = m.equal_range(6);
			assert(r.first == next(m.begin(), 1));
			assert(r.second == next(m.begin(), 1));
			r = m.equal_range(8);
			assert(r.first == next(m.begin(), 2));
			assert(r.second == next(m.begin(), 2));
			r = m.equal_range(10);
			assert(r.first == next(m.begin(), 3));
			assert(r.second == next(m.begin(), 3));
			r = m.equal_range(12);
			assert(r.first == next(m.begin(), 4));
			assert(r.second == next(m.begin(), 4));
			r = m.equal_range(14);
			assert(r.first == next(m.begin(), 5));
			assert(r.second == next(m.begin(), 5));
			r = m.equal_range(16);
			assert(r.first == next(m.begin(), 6));
			assert(r.second == next(m.begin(), 6));
			r = m.equal_range(18);
			assert(r.first == next(m.begin(), 7));
			assert(r.second == next(m.begin(), 7));
			r = m.equal_range(20);
			assert(r.first == next(m.begin(), 8));
			assert(r.second == next(m.begin(), 8));
		}
	}
#endif
#if TEST_STD_VER > 11
	{
		typedef std::pair<const int, double> V;
		typedef std::map<int, double, std::less<>> M;
		typedef std::pair<M::iterator, M::iterator> R;

		V ar[] = {V(5, 5),  V(7, 6),   V(9, 7),	  V(11, 8),
			  V(13, 9), V(15, 10), V(17, 11), V(19, 12)};
		M m(ar, ar + sizeof(ar) / sizeof(ar[0]));
		R r = m.equal_range(5);
		assert(r.first == next(m.begin(), 0));
		assert(r.second == next(m.begin(), 1));
		r = m.equal_range(7);
		assert(r.first == next(m.begin(), 1));
		assert(r.second == next(m.begin(), 2));
		r = m.equal_range(9);
		assert(r.first == next(m.begin(), 2));
		assert(r.second == next(m.begin(), 3));
		r = m.equal_range(11);
		assert(r.first == next(m.begin(), 3));
		assert(r.second == next(m.begin(), 4));
		r = m.equal_range(13);
		assert(r.first == next(m.begin(), 4));
		assert(r.second == next(m.begin(), 5));
		r = m.equal_range(15);
		assert(r.first == next(m.begin(), 5));
		assert(r.second == next(m.begin(), 6));
		r = m.equal_range(17);
		assert(r.first == next(m.begin(), 6));
		assert(r.second == next(m.begin(), 7));
		r = m.equal_range(19);
		assert(r.first == next(m.begin(), 7));
		assert(r.second == next(m.begin(), 8));
		r = m.equal_range(4);
		assert(r.first == next(m.begin(), 0));
		assert(r.second == next(m.begin(), 0));
		r = m.equal_range(6);
		assert(r.first == next(m.begin(), 1));
		assert(r.second == next(m.begin(), 1));
		r = m.equal_range(8);
		assert(r.first == next(m.begin(), 2));
		assert(r.second == next(m.begin(), 2));
		r = m.equal_range(10);
		assert(r.first == next(m.begin(), 3));
		assert(r.second == next(m.begin(), 3));
		r = m.equal_range(12);
		assert(r.first == next(m.begin(), 4));
		assert(r.second == next(m.begin(), 4));
		r = m.equal_range(14);
		assert(r.first == next(m.begin(), 5));
		assert(r.second == next(m.begin(), 5));
		r = m.equal_range(16);
		assert(r.first == next(m.begin(), 6));
		assert(r.second == next(m.begin(), 6));
		r = m.equal_range(18);
		assert(r.first == next(m.begin(), 7));
		assert(r.second == next(m.begin(), 7));
		r = m.equal_range(20);
		assert(r.first == next(m.begin(), 8));
		assert(r.second == next(m.begin(), 8));

		r = m.equal_range(C2Int(5));
		assert(r.first == next(m.begin(), 0));
		assert(r.second == next(m.begin(), 1));
		r = m.equal_range(C2Int(7));
		assert(r.first == next(m.begin(), 1));
		assert(r.second == next(m.begin(), 2));
		r = m.equal_range(C2Int(9));
		assert(r.first == next(m.begin(), 2));
		assert(r.second == next(m.begin(), 3));
		r = m.equal_range(C2Int(11));
		assert(r.first == next(m.begin(), 3));
		assert(r.second == next(m.begin(), 4));
		r = m.equal_range(C2Int(13));
		assert(r.first == next(m.begin(), 4));
		assert(r.second == next(m.begin(), 5));
		r = m.equal_range(C2Int(15));
		assert(r.first == next(m.begin(), 5));
		assert(r.second == next(m.begin(), 6));
		r = m.equal_range(C2Int(17));
		assert(r.first == next(m.begin(), 6));
		assert(r.second == next(m.begin(), 7));
		r = m.equal_range(C2Int(19));
		assert(r.first == next(m.begin(), 7));
		assert(r.second == next(m.begin(), 8));
		r = m.equal_range(C2Int(4));
		assert(r.first == next(m.begin(), 0));
		assert(r.second == next(m.begin(), 0));
		r = m.equal_range(C2Int(6));
		assert(r.first == next(m.begin(), 1));
		assert(r.second == next(m.begin(), 1));
		r = m.equal_range(C2Int(8));
		assert(r.first == next(m.begin(), 2));
		assert(r.second == next(m.begin(), 2));
		r = m.equal_range(C2Int(10));
		assert(r.first == next(m.begin(), 3));
		assert(r.second == next(m.begin(), 3));
		r = m.equal_range(C2Int(12));
		assert(r.first == next(m.begin(), 4));
		assert(r.second == next(m.begin(), 4));
		r = m.equal_range(C2Int(14));
		assert(r.first == next(m.begin(), 5));
		assert(r.second == next(m.begin(), 5));
		r = m.equal_range(C2Int(16));
		assert(r.first == next(m.begin(), 6));
		assert(r.second == next(m.begin(), 6));
		r = m.equal_range(C2Int(18));
		assert(r.first == next(m.begin(), 7));
		assert(r.second == next(m.begin(), 7));
		r = m.equal_range(C2Int(20));
		assert(r.first == next(m.begin(), 8));
		assert(r.second == next(m.begin(), 8));
	}
	{
		typedef PrivateConstructor PC;
		typedef std::map<PC, double, std::less<>> M;
		typedef std::pair<M::iterator, M::iterator> R;

		M m;
		m[PC::make(5)] = 5;
		m[PC::make(7)] = 6;
		m[PC::make(9)] = 7;
		m[PC::make(11)] = 8;
		m[PC::make(13)] = 9;
		m[PC::make(15)] = 10;
		m[PC::make(17)] = 11;
		m[PC::make(19)] = 12;

		R r = m.equal_range(5);
		assert(r.first == next(m.begin(), 0));
		assert(r.second == next(m.begin(), 1));
		r = m.equal_range(7);
		assert(r.first == next(m.begin(), 1));
		assert(r.second == next(m.begin(), 2));
		r = m.equal_range(9);
		assert(r.first == next(m.begin(), 2));
		assert(r.second == next(m.begin(), 3));
		r = m.equal_range(11);
		assert(r.first == next(m.begin(), 3));
		assert(r.second == next(m.begin(), 4));
		r = m.equal_range(13);
		assert(r.first == next(m.begin(), 4));
		assert(r.second == next(m.begin(), 5));
		r = m.equal_range(15);
		assert(r.first == next(m.begin(), 5));
		assert(r.second == next(m.begin(), 6));
		r = m.equal_range(17);
		assert(r.first == next(m.begin(), 6));
		assert(r.second == next(m.begin(), 7));
		r = m.equal_range(19);
		assert(r.first == next(m.begin(), 7));
		assert(r.second == next(m.begin(), 8));
		r = m.equal_range(4);
		assert(r.first == next(m.begin(), 0));
		assert(r.second == next(m.begin(), 0));
		r = m.equal_range(6);
		assert(r.first == next(m.begin(), 1));
		assert(r.second == next(m.begin(), 1));
		r = m.equal_range(8);
		assert(r.first == next(m.begin(), 2));
		assert(r.second == next(m.begin(), 2));
		r = m.equal_range(10);
		assert(r.first == next(m.begin(), 3));
		assert(r.second == next(m.begin(), 3));
		r = m.equal_range(12);
		assert(r.first == next(m.begin(), 4));
		assert(r.second == next(m.begin(), 4));
		r = m.equal_range(14);
		assert(r.first == next(m.begin(), 5));
		assert(r.second == next(m.begin(), 5));
		r = m.equal_range(16);
		assert(r.first == next(m.begin(), 6));
		assert(r.second == next(m.begin(), 6));
		r = m.equal_range(18);
		assert(r.first == next(m.begin(), 7));
		assert(r.second == next(m.begin(), 7));
		r = m.equal_range(20);
		assert(r.first == next(m.begin(), 8));
		assert(r.second == next(m.begin(), 8));
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
		pop = pmem::obj::pool<root>::create(path, "equal_range.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}
	try {
		pmem::obj::transaction::run(pop, [&] { run(pop); });
	} catch (std::exception &e) {
		UT_FATAL("!run: %s", e.what());
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
