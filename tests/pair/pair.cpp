/*
 * Copyright 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

int
main(int argc, char *argv[])
{
	START();

	construct_test();

	return 0;
}
