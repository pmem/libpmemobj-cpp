//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <string>

// template<class charT, class traits, class Allocator>
//   bool operator!=(const basic_string<charT, traits, Allocator> &lhs,
//   basic_string_view<charT,traits> rhs);
// template<class charT, class traits, class Allocator>
//   bool operator!=(basic_string_view<charT,traits> lhs, const
//   basic_string<charT, traits, Allocator> &rhs);

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>
#include <string>

template <class S>
void
test(const std::string &lhs, S rhs, bool x)
{
	UT_ASSERT((lhs != rhs) == x);
	UT_ASSERT((rhs != lhs) == x);
}

static void
run(int argc, char *argv[])
{
	{
		typedef pmem::obj::string_view S;
		test("", S(""), false);
		test("", S("abcde"), true);
		test("", S("abcdefghij"), true);
		test("", S("abcdefghijklmnopqrst"), true);
		test("abcde", S(""), true);
		test("abcde", S("abcde"), false);
		test("abcde", S("abcdefghij"), true);
		test("abcde", S("abcdefghijklmnopqrst"), true);
		test("abcdefghij", S(""), true);
		test("abcdefghij", S("abcde"), true);
		test("abcdefghij", S("abcdefghij"), false);
		test("abcdefghij", S("abcdefghijklmnopqrst"), true);
		test("abcdefghijklmnopqrst", S(""), true);
		test("abcdefghijklmnopqrst", S("abcde"), true);
		test("abcdefghijklmnopqrst", S("abcdefghij"), true);
		test("abcdefghijklmnopqrst", S("abcdefghijklmnopqrst"), false);
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(argc, argv); });
}
