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

// UNSUPPORTED: c++98, c++03

// <map>

// class map

// map(initializer_list<value_type> il, const key_compare& comp, const
// allocator_type& a);

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using CM = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<CM> s;
};

template <typename T, typename T2>
bool
operator==(const T &a, const T2 &b)
{
	if (a.first == b.first && a.second == b.second)
		return true;
	return false;
}

int
run(pmem::obj::pool<root> &pop)
{
#ifdef XXX // XXX: Implement test_compare, test_allocator and min_allocator
	{
		typedef std::pair<const int, double> V;
		typedef test_compare<std::less<int>> C;
		typedef test_allocator<std::pair<const int, double>> A;
		std::map<int, double, C, A> m({{1, 1},
					       {1, 1.5},
					       {1, 2},
					       {2, 1},
					       {2, 1.5},
					       {2, 2},
					       {3, 1},
					       {3, 1.5},
					       {3, 2}},
					      C(3), A(6));
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));
		UT_ASSERT(m.key_comp() == C(3));
		UT_ASSERT(m.get_allocator() == A(6));
	}
	{
		typedef std::pair<const int, double> V;
		typedef test_compare<std::less<int>> C;
		typedef min_allocator<std::pair<const int, double>> A;
		std::map<int, double, C, A> m({{1, 1},
					       {1, 1.5},
					       {1, 2},
					       {2, 1},
					       {2, 1.5},
					       {2, 2},
					       {3, 1},
					       {3, 1.5},
					       {3, 2}},
					      C(3), A());
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));
		UT_ASSERT(m.key_comp() == C(3));
		UT_ASSERT(m.get_allocator() == A());
	}
	{
		typedef std::pair<const int, double> V;
		typedef min_allocator<V> A;
		typedef test_compare<std::less<int>> C;
		typedef std::map<int, double, C, A> M;
		A a;
		M m({{1, 1},
		     {1, 1.5},
		     {1, 2},
		     {2, 1},
		     {2, 1.5},
		     {2, 2},
		     {3, 1},
		     {3, 1.5},
		     {3, 2}},
		    a);

		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));
		UT_ASSERT(m.get_allocator() == a);
	}
	{
		typedef std::pair<const int, double> V;
		typedef explicit_allocator<V> A;
		typedef test_compare<std::less<int>> C;
		A a;
		std::map<int, double, C, A> m({{1, 1},
					       {1, 1.5},
					       {1, 2},
					       {2, 1},
					       {2, 1.5},
					       {2, 2},
					       {3, 1},
					       {3, 1.5},
					       {3, 2}},
					      C(3), a);
		UT_ASSERT(m.size() == 3);
		UT_ASSERT(distance(m.begin(), m.end()) == 3);
		UT_ASSERT(*m.begin() == V(1, 1));
		UT_ASSERT(*next(m.begin()) == V(2, 1));
		UT_ASSERT(*next(m.begin(), 2) == V(3, 1));
		UT_ASSERT(m.key_comp() == C(3));
		UT_ASSERT(m.get_allocator() == a);
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
			path, "initializer_list_compare.pass", PMEMOBJ_MIN_POOL,
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
