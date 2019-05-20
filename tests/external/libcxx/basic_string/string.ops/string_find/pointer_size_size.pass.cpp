//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

using C = nvobj::string;

struct root {
	nvobj::persistent_ptr<C> s_arr[4];
};

template <class S>
void
test(const S &s, const typename S::value_type *str, typename S::size_type pos,
     typename S::size_type n, typename S::size_type x)
{
	UT_ASSERT(s.find(str, pos, n) == x);
	if (x != S::npos)
		UT_ASSERT(pos <= x && x + n <= s.size());
}

template <class S>
void
test0(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], "", 0, 0, 0);
	test(*s_arr[0], "abcde", 0, 0, 0);
	test(*s_arr[0], "abcde", 0, 1, S::npos);
	test(*s_arr[0], "abcde", 0, 2, S::npos);
	test(*s_arr[0], "abcde", 0, 4, S::npos);
	test(*s_arr[0], "abcde", 0, 5, S::npos);
	test(*s_arr[0], "abcdeabcde", 0, 0, 0);
	test(*s_arr[0], "abcdeabcde", 0, 1, S::npos);
	test(*s_arr[0], "abcdeabcde", 0, 5, S::npos);
	test(*s_arr[0], "abcdeabcde", 0, 9, S::npos);
	test(*s_arr[0], "abcdeabcde", 0, 10, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 0, 0, 0);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 0, 1, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 0, 10, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 0, 19, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 0, 20, S::npos);
	test(*s_arr[0], "", 1, 0, S::npos);
	test(*s_arr[0], "abcde", 1, 0, S::npos);
	test(*s_arr[0], "abcde", 1, 1, S::npos);
	test(*s_arr[0], "abcde", 1, 2, S::npos);
	test(*s_arr[0], "abcde", 1, 4, S::npos);
	test(*s_arr[0], "abcde", 1, 5, S::npos);
	test(*s_arr[0], "abcdeabcde", 1, 0, S::npos);
	test(*s_arr[0], "abcdeabcde", 1, 1, S::npos);
	test(*s_arr[0], "abcdeabcde", 1, 5, S::npos);
	test(*s_arr[0], "abcdeabcde", 1, 9, S::npos);
	test(*s_arr[0], "abcdeabcde", 1, 10, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 1, 0, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 1, 1, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 1, 10, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 1, 19, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 1, 20, S::npos);
	test(*s_arr[1], "", 0, 0, 0);
	test(*s_arr[1], "abcde", 0, 0, 0);
	test(*s_arr[1], "abcde", 0, 1, 0);
	test(*s_arr[1], "abcde", 0, 2, 0);
	test(*s_arr[1], "abcde", 0, 4, 0);
	test(*s_arr[1], "abcde", 0, 5, 0);
	test(*s_arr[1], "abcdeabcde", 0, 0, 0);
	test(*s_arr[1], "abcdeabcde", 0, 1, 0);
	test(*s_arr[1], "abcdeabcde", 0, 5, 0);
	test(*s_arr[1], "abcdeabcde", 0, 9, S::npos);
	test(*s_arr[1], "abcdeabcde", 0, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 0, 0, 0);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 0, 1, 0);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 0, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 0, 19, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 0, 20, S::npos);
	test(*s_arr[1], "", 1, 0, 1);
	test(*s_arr[1], "abcde", 1, 0, 1);
	test(*s_arr[1], "abcde", 1, 1, S::npos);
	test(*s_arr[1], "abcde", 1, 2, S::npos);
	test(*s_arr[1], "abcde", 1, 4, S::npos);
	test(*s_arr[1], "abcde", 1, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 1, 0, 1);
	test(*s_arr[1], "abcdeabcde", 1, 1, S::npos);
	test(*s_arr[1], "abcdeabcde", 1, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 1, 9, S::npos);
	test(*s_arr[1], "abcdeabcde", 1, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 1, 0, 1);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 1, 1, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 1, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 1, 19, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 1, 20, S::npos);
	test(*s_arr[1], "", 2, 0, 2);
	test(*s_arr[1], "abcde", 2, 0, 2);
	test(*s_arr[1], "abcde", 2, 1, S::npos);
	test(*s_arr[1], "abcde", 2, 2, S::npos);
	test(*s_arr[1], "abcde", 2, 4, S::npos);
	test(*s_arr[1], "abcde", 2, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 2, 0, 2);
	test(*s_arr[1], "abcdeabcde", 2, 1, S::npos);
	test(*s_arr[1], "abcdeabcde", 2, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 2, 9, S::npos);
	test(*s_arr[1], "abcdeabcde", 2, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 2, 0, 2);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 2, 1, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 2, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 2, 19, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 2, 20, S::npos);
	test(*s_arr[1], "", 4, 0, 4);
	test(*s_arr[1], "abcde", 4, 0, 4);
	test(*s_arr[1], "abcde", 4, 1, S::npos);
	test(*s_arr[1], "abcde", 4, 2, S::npos);
	test(*s_arr[1], "abcde", 4, 4, S::npos);
	test(*s_arr[1], "abcde", 4, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 4, 0, 4);
	test(*s_arr[1], "abcdeabcde", 4, 1, S::npos);
	test(*s_arr[1], "abcdeabcde", 4, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 4, 9, S::npos);
	test(*s_arr[1], "abcdeabcde", 4, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 4, 0, 4);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 4, 1, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 4, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 4, 19, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 4, 20, S::npos);
	test(*s_arr[1], "", 5, 0, 5);
	test(*s_arr[1], "abcde", 5, 0, 5);
	test(*s_arr[1], "abcde", 5, 1, S::npos);
	test(*s_arr[1], "abcde", 5, 2, S::npos);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[1], "abcde", 5, 4, S::npos);
	test(*s_arr[1], "abcde", 5, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 5, 0, 5);
	test(*s_arr[1], "abcdeabcde", 5, 1, S::npos);
	test(*s_arr[1], "abcdeabcde", 5, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 5, 9, S::npos);
	test(*s_arr[1], "abcdeabcde", 5, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 5, 0, 5);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 5, 1, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 5, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 5, 19, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 5, 20, S::npos);
	test(*s_arr[1], "", 6, 0, S::npos);
	test(*s_arr[1], "abcde", 6, 0, S::npos);
	test(*s_arr[1], "abcde", 6, 1, S::npos);
	test(*s_arr[1], "abcde", 6, 2, S::npos);
	test(*s_arr[1], "abcde", 6, 4, S::npos);
	test(*s_arr[1], "abcde", 6, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 6, 0, S::npos);
	test(*s_arr[1], "abcdeabcde", 6, 1, S::npos);
	test(*s_arr[1], "abcdeabcde", 6, 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 6, 9, S::npos);
	test(*s_arr[1], "abcdeabcde", 6, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 6, 0, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 6, 1, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 6, 10, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 6, 19, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 6, 20, S::npos);
	test(*s_arr[2], "", 0, 0, 0);
	test(*s_arr[2], "abcde", 0, 0, 0);
	test(*s_arr[2], "abcde", 0, 1, 0);
	test(*s_arr[2], "abcde", 0, 2, 0);
	test(*s_arr[2], "abcde", 0, 4, 0);
	test(*s_arr[2], "abcde", 0, 5, 0);
	test(*s_arr[2], "abcdeabcde", 0, 0, 0);
	test(*s_arr[2], "abcdeabcde", 0, 1, 0);
	test(*s_arr[2], "abcdeabcde", 0, 5, 0);
	test(*s_arr[2], "abcdeabcde", 0, 9, 0);
	test(*s_arr[2], "abcdeabcde", 0, 10, 0);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 0, 0, 0);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 0, 1, 0);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 0, 10, 0);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 0, 19, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 0, 20, S::npos);
	test(*s_arr[2], "", 1, 0, 1);
	test(*s_arr[2], "abcde", 1, 0, 1);
	test(*s_arr[2], "abcde", 1, 1, 5);
	test(*s_arr[2], "abcde", 1, 2, 5);
	test(*s_arr[2], "abcde", 1, 4, 5);
	test(*s_arr[2], "abcde", 1, 5, 5);
	test(*s_arr[2], "abcdeabcde", 1, 0, 1);
	test(*s_arr[2], "abcdeabcde", 1, 1, 5);
	test(*s_arr[2], "abcdeabcde", 1, 5, 5);
	test(*s_arr[2], "abcdeabcde", 1, 9, S::npos);
	test(*s_arr[2], "abcdeabcde", 1, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 1, 0, 1);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 1, 1, 5);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 1, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 1, 19, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 1, 20, S::npos);
	test(*s_arr[2], "", 5, 0, 5);
	test(*s_arr[2], "abcde", 5, 0, 5);
	test(*s_arr[2], "abcde", 5, 1, 5);
	test(*s_arr[2], "abcde", 5, 2, 5);
	test(*s_arr[2], "abcde", 5, 4, 5);
	test(*s_arr[2], "abcde", 5, 5, 5);
	test(*s_arr[2], "abcdeabcde", 5, 0, 5);
	test(*s_arr[2], "abcdeabcde", 5, 1, 5);
	test(*s_arr[2], "abcdeabcde", 5, 5, 5);
	test(*s_arr[2], "abcdeabcde", 5, 9, S::npos);
	test(*s_arr[2], "abcdeabcde", 5, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 5, 0, 5);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 5, 1, 5);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 5, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 5, 19, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 5, 20, S::npos);
	test(*s_arr[2], "", 9, 0, 9);
	test(*s_arr[2], "abcde", 9, 0, 9);
	test(*s_arr[2], "abcde", 9, 1, S::npos);
	test(*s_arr[2], "abcde", 9, 2, S::npos);
	test(*s_arr[2], "abcde", 9, 4, S::npos);
	test(*s_arr[2], "abcde", 9, 5, S::npos);
	test(*s_arr[2], "abcdeabcde", 9, 0, 9);
	test(*s_arr[2], "abcdeabcde", 9, 1, S::npos);
	test(*s_arr[2], "abcdeabcde", 9, 5, S::npos);
	test(*s_arr[2], "abcdeabcde", 9, 9, S::npos);
	test(*s_arr[2], "abcdeabcde", 9, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 9, 0, 9);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 9, 1, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 9, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 9, 19, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 9, 20, S::npos);
	test(*s_arr[2], "", 10, 0, 10);
	test(*s_arr[2], "abcde", 10, 0, 10);
	test(*s_arr[2], "abcde", 10, 1, S::npos);
	test(*s_arr[2], "abcde", 10, 2, S::npos);
	test(*s_arr[2], "abcde", 10, 4, S::npos);
	test(*s_arr[2], "abcde", 10, 5, S::npos);
	test(*s_arr[2], "abcdeabcde", 10, 0, 10);
	test(*s_arr[2], "abcdeabcde", 10, 1, S::npos);
}

template <class S>
void
test2(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[2], "abcdeabcde", 10, 5, S::npos);
	test(*s_arr[2], "abcdeabcde", 10, 9, S::npos);
	test(*s_arr[2], "abcdeabcde", 10, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 10, 0, 10);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 10, 1, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 10, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 10, 19, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 10, 20, S::npos);
	test(*s_arr[2], "", 11, 0, S::npos);
	test(*s_arr[2], "abcde", 11, 0, S::npos);
	test(*s_arr[2], "abcde", 11, 1, S::npos);
	test(*s_arr[2], "abcde", 11, 2, S::npos);
	test(*s_arr[2], "abcde", 11, 4, S::npos);
	test(*s_arr[2], "abcde", 11, 5, S::npos);
	test(*s_arr[2], "abcdeabcde", 11, 0, S::npos);
	test(*s_arr[2], "abcdeabcde", 11, 1, S::npos);
	test(*s_arr[2], "abcdeabcde", 11, 5, S::npos);
	test(*s_arr[2], "abcdeabcde", 11, 9, S::npos);
	test(*s_arr[2], "abcdeabcde", 11, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 11, 0, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 11, 1, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 11, 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 11, 19, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 11, 20, S::npos);
	test(*s_arr[3], "", 0, 0, 0);
	test(*s_arr[3], "abcde", 0, 0, 0);
	test(*s_arr[3], "abcde", 0, 1, 0);
	test(*s_arr[3], "abcde", 0, 2, 0);
	test(*s_arr[3], "abcde", 0, 4, 0);
	test(*s_arr[3], "abcde", 0, 5, 0);
	test(*s_arr[3], "abcdeabcde", 0, 0, 0);
	test(*s_arr[3], "abcdeabcde", 0, 1, 0);
	test(*s_arr[3], "abcdeabcde", 0, 5, 0);
	test(*s_arr[3], "abcdeabcde", 0, 9, 0);
	test(*s_arr[3], "abcdeabcde", 0, 10, 0);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 0, 0, 0);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 0, 1, 0);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 0, 10, 0);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 0, 19, 0);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 0, 20, 0);
	test(*s_arr[3], "", 1, 0, 1);
	test(*s_arr[3], "abcde", 1, 0, 1);
	test(*s_arr[3], "abcde", 1, 1, 5);
	test(*s_arr[3], "abcde", 1, 2, 5);
	test(*s_arr[3], "abcde", 1, 4, 5);
	test(*s_arr[3], "abcde", 1, 5, 5);
	test(*s_arr[3], "abcdeabcde", 1, 0, 1);
	test(*s_arr[3], "abcdeabcde", 1, 1, 5);
	test(*s_arr[3], "abcdeabcde", 1, 5, 5);
	test(*s_arr[3], "abcdeabcde", 1, 9, 5);
	test(*s_arr[3], "abcdeabcde", 1, 10, 5);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 1, 0, 1);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 1, 1, 5);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 1, 10, 5);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 1, 19, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 1, 20, S::npos);
	test(*s_arr[3], "", 10, 0, 10);
	test(*s_arr[3], "abcde", 10, 0, 10);
	test(*s_arr[3], "abcde", 10, 1, 10);
	test(*s_arr[3], "abcde", 10, 2, 10);
	test(*s_arr[3], "abcde", 10, 4, 10);
	test(*s_arr[3], "abcde", 10, 5, 10);
	test(*s_arr[3], "abcdeabcde", 10, 0, 10);
	test(*s_arr[3], "abcdeabcde", 10, 1, 10);
	test(*s_arr[3], "abcdeabcde", 10, 5, 10);
	test(*s_arr[3], "abcdeabcde", 10, 9, 10);
	test(*s_arr[3], "abcdeabcde", 10, 10, 10);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 10, 0, 10);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 10, 1, 10);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 10, 10, 10);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 10, 19, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 10, 20, S::npos);
	test(*s_arr[3], "", 19, 0, 19);
	test(*s_arr[3], "abcde", 19, 0, 19);
	test(*s_arr[3], "abcde", 19, 1, S::npos);
	test(*s_arr[3], "abcde", 19, 2, S::npos);
	test(*s_arr[3], "abcde", 19, 4, S::npos);
	test(*s_arr[3], "abcde", 19, 5, S::npos);
	test(*s_arr[3], "abcdeabcde", 19, 0, 19);
	test(*s_arr[3], "abcdeabcde", 19, 1, S::npos);
	test(*s_arr[3], "abcdeabcde", 19, 5, S::npos);
	test(*s_arr[3], "abcdeabcde", 19, 9, S::npos);
	test(*s_arr[3], "abcdeabcde", 19, 10, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 19, 0, 19);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 19, 1, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 19, 10, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 19, 19, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 19, 20, S::npos);
	test(*s_arr[3], "", 20, 0, 20);
	test(*s_arr[3], "abcde", 20, 0, 20);
	test(*s_arr[3], "abcde", 20, 1, S::npos);
	test(*s_arr[3], "abcde", 20, 2, S::npos);
	test(*s_arr[3], "abcde", 20, 4, S::npos);
	test(*s_arr[3], "abcde", 20, 5, S::npos);
	test(*s_arr[3], "abcdeabcde", 20, 0, 20);
	test(*s_arr[3], "abcdeabcde", 20, 1, S::npos);
	test(*s_arr[3], "abcdeabcde", 20, 5, S::npos);
	test(*s_arr[3], "abcdeabcde", 20, 9, S::npos);
	test(*s_arr[3], "abcdeabcde", 20, 10, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 20, 0, 20);
}

template <class S>
void
test3(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[3], "abcdeabcdeabcdeabcde", 20, 1, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 20, 10, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 20, 19, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 20, 20, S::npos);
	test(*s_arr[3], "", 21, 0, S::npos);
	test(*s_arr[3], "abcde", 21, 0, S::npos);
	test(*s_arr[3], "abcde", 21, 1, S::npos);
	test(*s_arr[3], "abcde", 21, 2, S::npos);
	test(*s_arr[3], "abcde", 21, 4, S::npos);
	test(*s_arr[3], "abcde", 21, 5, S::npos);
	test(*s_arr[3], "abcdeabcde", 21, 0, S::npos);
	test(*s_arr[3], "abcdeabcde", 21, 1, S::npos);
	test(*s_arr[3], "abcdeabcde", 21, 5, S::npos);
	test(*s_arr[3], "abcdeabcde", 21, 9, S::npos);
	test(*s_arr[3], "abcdeabcde", 21, 10, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 21, 0, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 21, 1, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 21, 10, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 21, 19, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 21, 20, S::npos);
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "string_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto &s_arr = pop.root()->s_arr;

	try {
		nvobj::transaction::run(pop, [&] {
			s_arr[0] = nvobj::make_persistent<C>("");
			s_arr[1] = nvobj::make_persistent<C>("abcde");
			s_arr[2] = nvobj::make_persistent<C>("abcdeabcde");
			s_arr[3] = nvobj::make_persistent<C>(
				"abcdeabcdeabcdeabcde");
		});

		test0<C>(pop);
		test1<C>(pop);
		test2<C>(pop);
		test3<C>(pop);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 4; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
