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
test(const S &s, const S &str, typename S::size_type pos,
     typename S::size_type x)
{
	UT_ASSERT(s.rfind(str, pos) == x);
	if (x != S::npos)
		UT_ASSERT(x <= pos && x + str.size() <= s.size());
}

template <class S>
void
test(const S &s, const S &str, typename S::size_type x)
{
	UT_ASSERT(s.rfind(str) == x);
	if (x != S::npos)
		UT_ASSERT(0 <= x && x + str.size() <= s.size());
}

template <class S>
void
test0(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], *s_arr[0], 0, 0);
	test(*s_arr[0], *s_arr[1], 0, S::npos);
	test(*s_arr[0], *s_arr[2], 0, S::npos);
	test(*s_arr[0], *s_arr[3], 0, S::npos);
	test(*s_arr[0], *s_arr[0], 1, 0);
	test(*s_arr[0], *s_arr[1], 1, S::npos);
	test(*s_arr[0], *s_arr[2], 1, S::npos);
	test(*s_arr[0], *s_arr[3], 1, S::npos);
	test(*s_arr[1], *s_arr[0], 0, 0);
	test(*s_arr[1], *s_arr[1], 0, 0);
	test(*s_arr[1], *s_arr[2], 0, S::npos);
	test(*s_arr[1], *s_arr[3], 0, S::npos);
	test(*s_arr[1], *s_arr[0], 1, 1);
	test(*s_arr[1], *s_arr[1], 1, 0);
	test(*s_arr[1], *s_arr[2], 1, S::npos);
	test(*s_arr[1], *s_arr[3], 1, S::npos);
	test(*s_arr[1], *s_arr[0], 2, 2);
	test(*s_arr[1], *s_arr[1], 2, 0);
	test(*s_arr[1], *s_arr[2], 2, S::npos);
	test(*s_arr[1], *s_arr[3], 2, S::npos);
	test(*s_arr[1], *s_arr[0], 4, 4);
	test(*s_arr[1], *s_arr[1], 4, 0);
	test(*s_arr[1], *s_arr[2], 4, S::npos);
	test(*s_arr[1], *s_arr[3], 4, S::npos);
	test(*s_arr[1], *s_arr[0], 5, 5);
	test(*s_arr[1], *s_arr[1], 5, 0);
	test(*s_arr[1], *s_arr[2], 5, S::npos);
	test(*s_arr[1], *s_arr[3], 5, S::npos);
	test(*s_arr[1], *s_arr[0], 6, 5);
	test(*s_arr[1], *s_arr[1], 6, 0);
	test(*s_arr[1], *s_arr[2], 6, S::npos);
	test(*s_arr[1], *s_arr[3], 6, S::npos);
	test(*s_arr[2], *s_arr[0], 0, 0);
	test(*s_arr[2], *s_arr[1], 0, 0);
	test(*s_arr[2], *s_arr[2], 0, 0);
	test(*s_arr[2], *s_arr[3], 0, S::npos);
	test(*s_arr[2], *s_arr[0], 1, 1);
	test(*s_arr[2], *s_arr[1], 1, 0);
	test(*s_arr[2], *s_arr[2], 1, 0);
	test(*s_arr[2], *s_arr[3], 1, S::npos);
	test(*s_arr[2], *s_arr[0], 5, 5);
	test(*s_arr[2], *s_arr[1], 5, 5);
	test(*s_arr[2], *s_arr[2], 5, 0);
	test(*s_arr[2], *s_arr[3], 5, S::npos);
	test(*s_arr[2], *s_arr[0], 9, 9);
	test(*s_arr[2], *s_arr[1], 9, 5);
	test(*s_arr[2], *s_arr[2], 9, 0);
	test(*s_arr[2], *s_arr[3], 9, S::npos);
	test(*s_arr[2], *s_arr[0], 10, 10);
	test(*s_arr[2], *s_arr[1], 10, 5);
	test(*s_arr[2], *s_arr[2], 10, 0);
	test(*s_arr[2], *s_arr[3], 10, S::npos);
	test(*s_arr[2], *s_arr[0], 11, 10);
	test(*s_arr[2], *s_arr[1], 11, 5);
	test(*s_arr[2], *s_arr[2], 11, 0);
	test(*s_arr[2], *s_arr[3], 11, S::npos);
	test(*s_arr[3], *s_arr[0], 0, 0);
	test(*s_arr[3], *s_arr[1], 0, 0);
	test(*s_arr[3], *s_arr[2], 0, 0);
	test(*s_arr[3], *s_arr[3], 0, 0);
	test(*s_arr[3], *s_arr[0], 1, 1);
	test(*s_arr[3], *s_arr[1], 1, 0);
	test(*s_arr[3], *s_arr[2], 1, 0);
	test(*s_arr[3], *s_arr[3], 1, 0);
	test(*s_arr[3], *s_arr[0], 10, 10);
	test(*s_arr[3], *s_arr[1], 10, 10);
	test(*s_arr[3], *s_arr[2], 10, 10);
	test(*s_arr[3], *s_arr[3], 10, 0);
	test(*s_arr[3], *s_arr[0], 19, 19);
	test(*s_arr[3], *s_arr[1], 19, 15);
	test(*s_arr[3], *s_arr[2], 19, 10);
	test(*s_arr[3], *s_arr[3], 19, 0);
	test(*s_arr[3], *s_arr[0], 20, 20);
	test(*s_arr[3], *s_arr[1], 20, 15);
	test(*s_arr[3], *s_arr[2], 20, 10);
	test(*s_arr[3], *s_arr[3], 20, 0);
	test(*s_arr[3], *s_arr[0], 21, 20);
	test(*s_arr[3], *s_arr[1], 21, 15);
	test(*s_arr[3], *s_arr[2], 21, 10);
	test(*s_arr[3], *s_arr[3], 21, 0);
}

template <class S>
void
test1(nvobj::pool<struct root> &pop)
{
	auto &s_arr = pop.root()->s_arr;

	test(*s_arr[0], *s_arr[0], 0);
	test(*s_arr[0], *s_arr[1], S::npos);
	test(*s_arr[0], *s_arr[2], S::npos);
	test(*s_arr[0], *s_arr[3], S::npos);
	test(*s_arr[1], *s_arr[0], 5);
	test(*s_arr[1], *s_arr[1], 0);
	test(*s_arr[1], *s_arr[2], S::npos);
	test(*s_arr[1], *s_arr[3], S::npos);
	test(*s_arr[2], *s_arr[0], 10);
	test(*s_arr[2], *s_arr[1], 5);
	test(*s_arr[2], *s_arr[2], 0);
	test(*s_arr[2], *s_arr[3], S::npos);
	test(*s_arr[3], *s_arr[0], 20);
	test(*s_arr[3], *s_arr[1], 15);
	test(*s_arr[3], *s_arr[2], 10);
	test(*s_arr[3], *s_arr[3], 0);
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
