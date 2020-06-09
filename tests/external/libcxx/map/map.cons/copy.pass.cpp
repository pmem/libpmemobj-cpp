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

// map(const map& m);

#include "unittest.hpp"

#include <libpmemobj++/experimental/concurrent_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using CM = nvobjex::concurrent_map<int, double>;

struct root {
	nvobj::persistent_ptr<CM> s;
};

int
run(pmem::obj::pool<root> &pop)
{
#ifdef XXX // Implement test_compare and test_allocator classes
	{
		typedef std::pair<const int, double> V;
		V ar[] = {
			V(1, 1), V(1, 1.5), V(1, 2),   V(2, 1), V(2, 1.5),
			V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2),
		};
		typedef test_compare<std::less<int>> C;
		typedef test_allocator<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A(7));
		std::map<int, double, C, A> m = mo;
		assert(m.get_allocator() == A(7));
		assert(m.key_comp() == C(5));
		assert(m.size() == 3);
		assert(distance(m.begin(), m.end()) == 3);
		assert(*m.begin() == V(1, 1));
		assert(*next(m.begin()) == V(2, 1));
		assert(*next(m.begin(), 2) == V(3, 1));

		assert(mo.get_allocator() == A(7));
		assert(mo.key_comp() == C(5));
		assert(mo.size() == 3);
		assert(distance(mo.begin(), mo.end()) == 3);
		assert(*mo.begin() == V(1, 1));
		assert(*next(mo.begin()) == V(2, 1));
		assert(*next(mo.begin(), 2) == V(3, 1));
	}
#endif
#if TEST_STD_VER >= 11
	{
		typedef std::pair<const int, double> V;
		V ar[] = {
			V(1, 1), V(1, 1.5), V(1, 2),   V(2, 1), V(2, 1.5),
			V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2),
		};
		typedef test_compare<std::less<int>> C;
		typedef other_allocator<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A(7));
		std::map<int, double, C, A> m = mo;
		assert(m.get_allocator() == A(-2));
		assert(m.key_comp() == C(5));
		assert(m.size() == 3);
		assert(distance(m.begin(), m.end()) == 3);
		assert(*m.begin() == V(1, 1));
		assert(*next(m.begin()) == V(2, 1));
		assert(*next(m.begin(), 2) == V(3, 1));

		assert(mo.get_allocator() == A(7));
		assert(mo.key_comp() == C(5));
		assert(mo.size() == 3);
		assert(distance(mo.begin(), mo.end()) == 3);
		assert(*mo.begin() == V(1, 1));
		assert(*next(mo.begin()) == V(2, 1));
		assert(*next(mo.begin(), 2) == V(3, 1));
	}
	{
		typedef std::pair<const int, double> V;
		V ar[] = {
			V(1, 1), V(1, 1.5), V(1, 2),   V(2, 1), V(2, 1.5),
			V(2, 2), V(3, 1),   V(3, 1.5), V(3, 2),
		};
		typedef test_compare<std::less<int>> C;
		typedef min_allocator<V> A;
		std::map<int, double, C, A> mo(
			ar, ar + sizeof(ar) / sizeof(ar[0]), C(5), A());
		std::map<int, double, C, A> m = mo;
		assert(m.get_allocator() == A());
		assert(m.key_comp() == C(5));
		assert(m.size() == 3);
		assert(distance(m.begin(), m.end()) == 3);
		assert(*m.begin() == V(1, 1));
		assert(*next(m.begin()) == V(2, 1));
		assert(*next(m.begin(), 2) == V(3, 1));

		assert(mo.get_allocator() == A());
		assert(mo.key_comp() == C(5));
		assert(mo.size() == 3);
		assert(distance(mo.begin(), mo.end()) == 3);
		assert(*mo.begin() == V(1, 1));
		assert(*next(mo.begin()) == V(2, 1));
		assert(*next(mo.begin(), 2) == V(3, 1));
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
			path, "copy.pass", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
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
