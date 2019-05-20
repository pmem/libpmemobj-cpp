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
test(const S &s, typename S::value_type c, typename S::size_type pos,
     typename S::size_type x)
{
	UT_ASSERT(s.rfind(c, pos) == x);
	if (x != S::npos)
		UT_ASSERT(x <= pos && x + 1 <= s.size());
}

template <class S>
void
test(const S &s, typename S::value_type c, typename S::size_type x)
{
	UT_ASSERT(s.rfind(c) == x);
	if (x != S::npos)
		UT_ASSERT(x + 1 <= s.size());
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

		test(*s_arr[0], 'b', 0, C::npos);
		test(*s_arr[0], 'b', 1, C::npos);
		test(*s_arr[1], 'b', 0, C::npos);
		test(*s_arr[1], 'b', 1, 1);
		test(*s_arr[1], 'b', 2, 1);
		test(*s_arr[1], 'b', 4, 1);
		test(*s_arr[1], 'b', 5, 1);
		test(*s_arr[1], 'b', 6, 1);
		test(*s_arr[2], 'b', 0, C::npos);
		test(*s_arr[2], 'b', 1, 1);
		test(*s_arr[2], 'b', 5, 1);
		test(*s_arr[2], 'b', 9, 6);
		test(*s_arr[2], 'b', 10, 6);
		test(*s_arr[2], 'b', 11, 6);
		test(*s_arr[3], 'b', 0, C::npos);
		test(*s_arr[3], 'b', 1, 1);
		test(*s_arr[3], 'b', 10, 6);
		test(*s_arr[3], 'b', 19, 16);
		test(*s_arr[3], 'b', 20, 16);
		test(*s_arr[3], 'b', 21, 16);

		test(*s_arr[0], 'b', C::npos);
		test(*s_arr[1], 'b', 1);
		test(*s_arr[2], 'b', 6);
		test(*s_arr[3], 'b', 16);

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
