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
test(const S &s, typename S::value_type c, typename S::size_type pos,
     typename S::size_type x)
{
	UT_ASSERT(s.find(c, pos) == x);
	if (x != S::npos)
		UT_ASSERT(pos <= x && x + 1 <= s.size());
}

template <class S>
void
test(const S &s, typename S::value_type c, typename S::size_type x)
{
	UT_ASSERT(s.find(c) == x);
	if (x != S::npos)
		UT_ASSERT(0 <= x && x + 1 <= s.size());
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

		test(*s_arr[0], 'c', 0, C::npos);
		test(*s_arr[0], 'c', 1, C::npos);
		test(*s_arr[1], 'c', 0, 2);
		test(*s_arr[1], 'c', 1, 2);
		test(*s_arr[1], 'c', 2, 2);
		test(*s_arr[1], 'c', 4, C::npos);
		test(*s_arr[1], 'c', 5, C::npos);
		test(*s_arr[1], 'c', 6, C::npos);
		test(*s_arr[2], 'c', 0, 2);
		test(*s_arr[2], 'c', 1, 2);
		test(*s_arr[2], 'c', 5, 7);
		test(*s_arr[2], 'c', 9, C::npos);
		test(*s_arr[2], 'c', 10, C::npos);
		test(*s_arr[2], 'c', 11, C::npos);
		test(*s_arr[3], 'c', 0, 2);
		test(*s_arr[3], 'c', 1, 2);
		test(*s_arr[3], 'c', 10, 12);
		test(*s_arr[3], 'c', 19, C::npos);
		test(*s_arr[3], 'c', 20, C::npos);
		test(*s_arr[3], 'c', 21, C::npos);

		test(*s_arr[0], 'c', C::npos);
		test(*s_arr[1], 'c', 2);
		test(*s_arr[2], 'c', 2);
		test(*s_arr[3], 'c', 2);

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
