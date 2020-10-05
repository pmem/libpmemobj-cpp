//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <string>

// template<class charT, class traits, class Allocator>
//   bool operator==(const charT* lhs, const basic_string<charT,traits> rhs);
// template<class charT, class traits, class Allocator>
//   bool operator==(const basic_string_view<charT,traits> lhs, const CharT*
//   rhs);

#include "unittest.hpp"

#include <string>

#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(const std::string &lhs, S rhs, bool x)
{
	UT_ASSERT((lhs == rhs) == x);
	UT_ASSERT((rhs == lhs) == x);
}

static void
run(int argc, char *argv[])
{
	{
		typedef pmem::obj::string_view S;
		test("", S(""), true);
		test("", S("abcde"), false);
		test("", S("abcdefghij"), false);
		test("", S("abcdefghijklmnopqrst"), false);
		test("abcde", S(""), false);
		test("abcde", S("abcde"), true);
		test("abcde", S("abcdefghij"), false);
		test("abcde", S("abcdefghijklmnopqrst"), false);
		test("abcdefghij", S(""), false);
		test("abcdefghij", S("abcde"), false);
		test("abcdefghij", S("abcdefghij"), true);
		test("abcdefghij", S("abcdefghijklmnopqrst"), false);
		test("abcdefghijklmnopqrst", S(""), false);
		test("abcdefghijklmnopqrst", S("abcde"), false);
		test("abcdefghijklmnopqrst", S("abcdefghij"), false);
		test("abcdefghijklmnopqrst", S("abcdefghijklmnopqrst"), true);
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(argc, argv); });
}
