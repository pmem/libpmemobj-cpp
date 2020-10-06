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
test(const std::basic_string<typename S::value_type> &lhs, S rhs, bool x)
{
	UT_ASSERT((lhs == rhs) == x);
	UT_ASSERT((rhs == lhs) == x);
}

static void
run()
{
	{
		typedef pmem::obj::basic_string_view<char> S;
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

static void
run_wchar_t()
{
	{
		typedef pmem::obj::basic_string_view<wchar_t> S;
		test(L"", S(L""), true);
		test(L"", S(L"abcde"), false);
		test(L"", S(L"abcdefghij"), false);
		test(L"", S(L"abcdefghijklmnopqrst"), false);
		test(L"abcde", S(L""), false);
		test(L"abcde", S(L"abcde"), true);
		test(L"abcde", S(L"abcdefghij"), false);
		test(L"abcde", S(L"abcdefghijklmnopqrst"), false);
		test(L"abcdefghij", S(L""), false);
		test(L"abcdefghij", S(L"abcde"), false);
		test(L"abcdefghij", S(L"abcdefghij"), true);
		test(L"abcdefghij", S(L"abcdefghijklmnopqrst"), false);
		test(L"abcdefghijklmnopqrst", S(L""), false);
		test(L"abcdefghijklmnopqrst", S(L"abcde"), false);
		test(L"abcdefghijklmnopqrst", S(L"abcdefghij"), false);
		test(L"abcdefghijklmnopqrst", S(L"abcdefghijklmnopqrst"), true);
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] {
		run();
		run_wchar_t();
	});
}
