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
//   bool operator>=(const basic_string<charT,traits,Allocator>& lhs,
//                   basic_string_view<charT,traits> rhs);
//   bool operator>=(basic_string_view<charT,traits> lhs,
//            const basic_string<charT,traits,Allocator>&  rhs);

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <class S>
static void
test(const S &lhs, const typename S::value_type *rhs, bool x, bool y)
{
	UT_ASSERT((lhs >= rhs) == x);
	UT_ASSERT((rhs >= lhs) == y);
}

static void
run()
{
	{
		typedef pmem::obj::basic_string_view<char> S;
		test(S(""), "", true, true);
		test(S(""), "abcde", false, true);
		test(S(""), "abcdefghij", false, true);
		test(S(""), "abcdefghijklmnopqrst", false, true);
		test(S("abcde"), "", true, false);
		test(S("abcde"), "abcde", true, true);
		test(S("abcde"), "abcdefghij", false, true);
		test(S("abcde"), "abcdefghijklmnopqrst", false, true);
		test(S("abcdefghij"), "", true, false);
		test(S("abcdefghij"), "abcde", true, false);
		test(S("abcdefghij"), "abcdefghij", true, true);
		test(S("abcdefghij"), "abcdefghijklmnopqrst", false, true);
		test(S("abcdefghijklmnopqrst"), "", true, false);
		test(S("abcdefghijklmnopqrst"), "abcde", true, false);
		test(S("abcdefghijklmnopqrst"), "abcdefghij", true, false);
		test(S("abcdefghijklmnopqrst"), "abcdefghijklmnopqrst", true,
		     true);
	}
}

static void
run_wchar_t()
{
	{
		typedef pmem::obj::basic_string_view<wchar_t> S;
		test(S(L""), L"", true, true);
		test(S(L""), L"abcde", false, true);
		test(S(L""), L"abcdefghij", false, true);
		test(S(L""), L"abcdefghijklmnopqrst", false, true);
		test(S(L"abcde"), L"", true, false);
		test(S(L"abcde"), L"abcde", true, true);
		test(S(L"abcde"), L"abcdefghij", false, true);
		test(S(L"abcde"), L"abcdefghijklmnopqrst", false, true);
		test(S(L"abcdefghij"), L"", true, false);
		test(S(L"abcdefghij"), L"abcde", true, false);
		test(S(L"abcdefghij"), L"abcdefghij", true, true);
		test(S(L"abcdefghij"), L"abcdefghijklmnopqrst", false, true);
		test(S(L"abcdefghijklmnopqrst"), L"", true, false);
		test(S(L"abcdefghijklmnopqrst"), L"abcde", true, false);
		test(S(L"abcdefghijklmnopqrst"), L"abcdefghij", true, false);
		test(S(L"abcdefghijklmnopqrst"), L"abcdefghijklmnopqrst", true,
		     true);
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
