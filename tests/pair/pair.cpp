// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <array>

#include <libpmemobj++/detail/pair.hpp>
#include <libpmemobj++/pool.hpp>

static int copy_ctor_called = 0;
static int move_ctor_called = 0;

struct A {
	A(int a = 0, int b = 0, int c = 0) : a(a), b(b), c(c)
	{
	}

	A(const A &rhs)
	{
		copy_ctor_called++;

		a = rhs.a;
		b = rhs.b;
		c = rhs.c;
	}

	A(A &&rhs)
	{
		move_ctor_called++;

		a = rhs.a;
		b = rhs.b;
		c = rhs.c;
	}

	A &operator=(const A &) = default;

	int a;
	int b;
	int c;
};

void
verify_vals(pmem::detail::pair<A, A> &a, std::array<int, 6> expected)
{
	UT_ASSERTeq(a.first.a, expected[0]);
	UT_ASSERTeq(a.first.b, expected[1]);
	UT_ASSERTeq(a.first.c, expected[2]);
	UT_ASSERTeq(a.second.a, expected[3]);
	UT_ASSERTeq(a.second.b, expected[4]);
	UT_ASSERTeq(a.second.c, expected[5]);
}

void
construct_test()
{
	{
		A a1(1, 2, 3), a2(4, 5, 6);
		copy_ctor_called = 0;
		move_ctor_called = 0;

		pmem::detail::pair<A, A> p(a1, a2);
		UT_ASSERTeq(copy_ctor_called, 2);
		UT_ASSERTeq(move_ctor_called, 0);

		verify_vals(p, {1, 2, 3, 4, 5, 6});
	}

	{
		A a1(1, 2, 3), a2(4, 5, 6);
		copy_ctor_called = 0;
		move_ctor_called = 0;

		pmem::detail::pair<A, A> p(std::move(a1), std::move(a2));
		UT_ASSERTeq(copy_ctor_called, 0);
		UT_ASSERTeq(move_ctor_called, 2);

		verify_vals(p, {1, 2, 3, 4, 5, 6});
	}

	{
		A a1(1, 2, 3), a2(4, 5, 6);
		copy_ctor_called = 0;
		move_ctor_called = 0;

		pmem::detail::pair<A, A> p(std::piecewise_construct,
					   std::forward_as_tuple(a1),
					   std::forward_as_tuple(a2));
		UT_ASSERTeq(copy_ctor_called, 2);
		UT_ASSERTeq(move_ctor_called, 0);

		verify_vals(p, {1, 2, 3, 4, 5, 6});
	}

	{
		A a1(1, 2, 3), a2(4, 5, 6);
		copy_ctor_called = 0;
		move_ctor_called = 0;

		pmem::detail::pair<A, A> p(
			std::piecewise_construct,
			std::forward_as_tuple(std::move(a1)),
			std::forward_as_tuple(std::move(a2)));
		UT_ASSERTeq(copy_ctor_called, 0);
		UT_ASSERTeq(move_ctor_called, 2);

		verify_vals(p, {1, 2, 3, 4, 5, 6});
	}

	{
		A a1(1, 2, 3), a2(4, 5, 6);
		copy_ctor_called = 0;
		move_ctor_called = 0;

		pmem::detail::pair<A, A> p(std::piecewise_construct,
					   std::forward_as_tuple(1, 2),
					   std::forward_as_tuple(3));
		UT_ASSERTeq(copy_ctor_called, 0);
		UT_ASSERTeq(move_ctor_called, 0);

		verify_vals(p, {1, 2, 0, 3, 0, 0});
	}
}

static void
test(int argc, char *argv[])
{
	construct_test();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
