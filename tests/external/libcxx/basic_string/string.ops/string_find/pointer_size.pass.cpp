//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
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
     typename S::size_type x)
{
	UT_ASSERT(s.find(str, pos) == x);
	if (x != S::npos) {
		typename S::size_type n = S::traits_type::length(str);
		UT_ASSERT(pos <= x && x + n <= s.size());
	}
}

template <class S>
void
test(const S &s, const typename S::value_type *str, typename S::size_type x)
{
	UT_ASSERT(s.find(str) == x);
	if (x != S::npos) {
		typename S::size_type n = S::traits_type::length(str);
		UT_ASSERT(0 <= x && x + n <= s.size());
	}
}

template <class S>
void
test0(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], "", 0, 0);
	test(*s_arr[0], "abcde", 0, S::npos);
	test(*s_arr[0], "abcdeabcde", 0, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 0, S::npos);
	test(*s_arr[0], "", 1, S::npos);
	test(*s_arr[0], "abcde", 1, S::npos);
	test(*s_arr[0], "abcdeabcde", 1, S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", 1, S::npos);
	test(*s_arr[1], "", 0, 0);
	test(*s_arr[1], "abcde", 0, 0);
	test(*s_arr[1], "abcdeabcde", 0, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 0, S::npos);
	test(*s_arr[1], "", 1, 1);
	test(*s_arr[1], "abcde", 1, S::npos);
	test(*s_arr[1], "abcdeabcde", 1, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 1, S::npos);
	test(*s_arr[1], "", 2, 2);
	test(*s_arr[1], "abcde", 2, S::npos);
	test(*s_arr[1], "abcdeabcde", 2, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 2, S::npos);
	test(*s_arr[1], "", 4, 4);
	test(*s_arr[1], "abcde", 4, S::npos);
	test(*s_arr[1], "abcdeabcde", 4, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 4, S::npos);
	test(*s_arr[1], "", 5, 5);
	test(*s_arr[1], "abcde", 5, S::npos);
	test(*s_arr[1], "abcdeabcde", 5, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 5, S::npos);
	test(*s_arr[1], "", 6, S::npos);
	test(*s_arr[1], "abcde", 6, S::npos);
	test(*s_arr[1], "abcdeabcde", 6, S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", 6, S::npos);
	test(*s_arr[2], "", 0, 0);
	test(*s_arr[2], "abcde", 0, 0);
	test(*s_arr[2], "abcdeabcde", 0, 0);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 0, S::npos);
	test(*s_arr[2], "", 1, 1);
	test(*s_arr[2], "abcde", 1, 5);
	test(*s_arr[2], "abcdeabcde", 1, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 1, S::npos);
	test(*s_arr[2], "", 5, 5);
	test(*s_arr[2], "abcde", 5, 5);
	test(*s_arr[2], "abcdeabcde", 5, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 5, S::npos);
	test(*s_arr[2], "", 9, 9);
	test(*s_arr[2], "abcde", 9, S::npos);
	test(*s_arr[2], "abcdeabcde", 9, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 9, S::npos);
	test(*s_arr[2], "", 10, 10);
	test(*s_arr[2], "abcde", 10, S::npos);
	test(*s_arr[2], "abcdeabcde", 10, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 10, S::npos);
	test(*s_arr[2], "", 11, S::npos);
	test(*s_arr[2], "abcde", 11, S::npos);
	test(*s_arr[2], "abcdeabcde", 11, S::npos);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", 11, S::npos);
	test(*s_arr[3], "", 0, 0);
	test(*s_arr[3], "abcde", 0, 0);
	test(*s_arr[3], "abcdeabcde", 0, 0);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 0, 0);
	test(*s_arr[3], "", 1, 1);
	test(*s_arr[3], "abcde", 1, 5);
	test(*s_arr[3], "abcdeabcde", 1, 5);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 1, S::npos);
	test(*s_arr[3], "", 10, 10);
	test(*s_arr[3], "abcde", 10, 10);
	test(*s_arr[3], "abcdeabcde", 10, 10);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 10, S::npos);
	test(*s_arr[3], "", 19, 19);
	test(*s_arr[3], "abcde", 19, S::npos);
	test(*s_arr[3], "abcdeabcde", 19, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 19, S::npos);
	test(*s_arr[3], "", 20, 20);
	test(*s_arr[3], "abcde", 20, S::npos);
	test(*s_arr[3], "abcdeabcde", 20, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 20, S::npos);
	test(*s_arr[3], "", 21, S::npos);
	test(*s_arr[3], "abcde", 21, S::npos);
	test(*s_arr[3], "abcdeabcde", 21, S::npos);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 21, S::npos);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], "", 0);
	test(*s_arr[0], "abcde", S::npos);
	test(*s_arr[0], "abcdeabcde", S::npos);
	test(*s_arr[0], "abcdeabcdeabcdeabcde", S::npos);
	test(*s_arr[1], "", 0);
	test(*s_arr[1], "abcde", 0);
	test(*s_arr[1], "abcdeabcde", S::npos);
	test(*s_arr[1], "abcdeabcdeabcdeabcde", S::npos);
	test(*s_arr[2], "", 0);
	test(*s_arr[2], "abcde", 0);
	test(*s_arr[2], "abcdeabcde", 0);
	test(*s_arr[2], "abcdeabcdeabcdeabcde", S::npos);
	test(*s_arr[3], "", 0);
	test(*s_arr[3], "abcde", 0);
	test(*s_arr[3], "abcdeabcde", 0);
	test(*s_arr[3], "abcdeabcdeabcdeabcde", 0);
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
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

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 4; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
