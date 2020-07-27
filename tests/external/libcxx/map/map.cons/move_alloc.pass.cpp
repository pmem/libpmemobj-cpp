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

// map(map&& m, const allocator_type& a);

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using C = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<C> s;
};

int
run(pmem::obj::pool<root> &pop)
{
#ifdef XXX // XXX: Implement test_allocator, other_allocator, min_allocator and
	   // explicit_allocator
	{
		typedef std::pair<MoveOnly, MoveOnly> V;
		typedef std::pair<const MoveOnly, MoveOnly> VC;
		typedef test_compare<std::less<MoveOnly>> C;
		typedef test_allocator<VC> A;
		typedef std::map<MoveOnly, MoveOnly, C, A> M;
		typedef std::move_iterator<V *> I;
		V a1[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m1(I(a1), I(a1 + sizeof(a1) / sizeof(a1[0])), C(5), A(7));
		V a2[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m2(I(a2), I(a2 + sizeof(a2) / sizeof(a2[0])), C(5), A(7));
		M m3(std::move(m1), A(7));
		UT_ASSERT(m3 == m2);
		UT_ASSERT(m3.get_allocator() == A(7));
		UT_ASSERT(m3.key_comp() == C(5));
		LIBCPP_ASSERT(m1.empty());
	}
	{
		typedef std::pair<MoveOnly, MoveOnly> V;
		typedef std::pair<const MoveOnly, MoveOnly> VC;
		typedef test_compare<std::less<MoveOnly>> C;
		typedef test_allocator<VC> A;
		typedef std::map<MoveOnly, MoveOnly, C, A> M;
		typedef std::move_iterator<V *> I;
		V a1[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m1(I(a1), I(a1 + sizeof(a1) / sizeof(a1[0])), C(5), A(7));
		V a2[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m2(I(a2), I(a2 + sizeof(a2) / sizeof(a2[0])), C(5), A(7));
		M m3(std::move(m1), A(5));
		UT_ASSERT(m3 == m2);
		UT_ASSERT(m3.get_allocator() == A(5));
		UT_ASSERT(m3.key_comp() == C(5));
		LIBCPP_ASSERT(m1.empty());
	}
	{
		typedef std::pair<MoveOnly, MoveOnly> V;
		typedef std::pair<const MoveOnly, MoveOnly> VC;
		typedef test_compare<std::less<MoveOnly>> C;
		typedef other_allocator<VC> A;
		typedef std::map<MoveOnly, MoveOnly, C, A> M;
		typedef std::move_iterator<V *> I;
		V a1[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m1(I(a1), I(a1 + sizeof(a1) / sizeof(a1[0])), C(5), A(7));
		V a2[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m2(I(a2), I(a2 + sizeof(a2) / sizeof(a2[0])), C(5), A(7));
		M m3(std::move(m1), A(5));
		UT_ASSERT(m3 == m2);
		UT_ASSERT(m3.get_allocator() == A(5));
		UT_ASSERT(m3.key_comp() == C(5));
		LIBCPP_ASSERT(m1.empty());
	}
	{
		typedef Counter<int> T;
		typedef std::pair<int, T> V;
		typedef std::pair<const int, T> VC;
		typedef test_allocator<VC> A;
		typedef std::less<int> C;
		typedef std::map<const int, T, C, A> M;
		typedef V *I;
		Counter_base::gConstructed = 0;
		{
			V a1[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
				  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
			const size_t num = sizeof(a1) / sizeof(a1[0]);
			UT_ASSERT(Counter_base::gConstructed == num);

			M m1(I(a1), I(a1 + num), C(), A());
			UT_ASSERT(Counter_base::gConstructed == num + 3);

			M m2(m1);
			UT_ASSERT(m2 == m1);
			UT_ASSERT(Counter_base::gConstructed == num + 6);

			M m3(std::move(m1), A());
			UT_ASSERT(m3 == m2);
			LIBCPP_ASSERT(m1.empty());
			UT_ASSERT(Counter_base::gConstructed >= (int)(num + 6));
			UT_ASSERT(Counter_base::gConstructed <=
				  (int)(num + 6 + m1.size()));

			{
				M m4(std::move(m2), A(5));
				UT_ASSERT(Counter_base::gConstructed >=
					  (int)(num + 6));
				UT_ASSERT(
					Counter_base::gConstructed <=
					(int)(num + 6 + m1.size() + m2.size()));
				UT_ASSERT(m4 == m3);
				LIBCPP_ASSERT(m2.empty());
			}
			UT_ASSERT(Counter_base::gConstructed >= (int)(num + 3));
			UT_ASSERT(Counter_base::gConstructed <=
				  (int)(num + 3 + m1.size() + m2.size()));
		}
		UT_ASSERT(Counter_base::gConstructed == 0);
	}
	{
		typedef std::pair<MoveOnly, MoveOnly> V;
		typedef std::pair<const MoveOnly, MoveOnly> VC;
		typedef test_compare<std::less<MoveOnly>> C;
		typedef min_allocator<VC> A;
		typedef std::map<MoveOnly, MoveOnly, C, A> M;
		typedef std::move_iterator<V *> I;
		V a1[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m1(I(a1), I(a1 + sizeof(a1) / sizeof(a1[0])), C(5), A());
		V a2[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m2(I(a2), I(a2 + sizeof(a2) / sizeof(a2[0])), C(5), A());
		M m3(std::move(m1), A());
		UT_ASSERT(m3 == m2);
		UT_ASSERT(m3.get_allocator() == A());
		UT_ASSERT(m3.key_comp() == C(5));
		LIBCPP_ASSERT(m1.empty());
	}
	{
		typedef std::pair<MoveOnly, MoveOnly> V;
		typedef std::pair<const MoveOnly, MoveOnly> VC;
		typedef test_compare<std::less<MoveOnly>> C;
		typedef explicit_allocator<VC> A;
		typedef std::map<MoveOnly, MoveOnly, C, A> M;
		typedef std::move_iterator<V *> I;
		V a1[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m1(I(a1), I(a1 + sizeof(a1) / sizeof(a1[0])), C(5), A{});
		V a2[] = {V(1, 1), V(1, 2), V(1, 3), V(2, 1), V(2, 2),
			  V(2, 3), V(3, 1), V(3, 2), V(3, 3)};
		M m2(I(a2), I(a2 + sizeof(a2) / sizeof(a2[0])), C(5), A{});
		M m3(std::move(m1), A{});
		UT_ASSERT(m3 == m2);
		UT_ASSERT(m3.get_allocator() == A{});
		UT_ASSERT(m3.key_comp() == C(5));
		LIBCPP_ASSERT(m1.empty());
	}

	return 0;
#endif
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(path, "move_alloc.pass",
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
