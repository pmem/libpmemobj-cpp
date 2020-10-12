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
//   constexpr bool operator>=(const charT* lhs, basic_string_wiew<charT,traits>
//   rhs);
// template<class charT, class traits, class Allocator>
//   constexpr bool operator>=(basic_string_wiew<charT,traits> lhs, const charT*
//   rhs);

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <class S>
static void
test(const typename S::value_type *lhs, const S &rhs, bool x, bool y)
{
	UT_ASSERT((lhs >= rhs) == x);
	UT_ASSERT((rhs >= lhs) == y);
}

static void
run(int argc, char *argv[])
{
	{
		typedef pmem::obj::string_view S;
		test("", S(""), true, true);
		test("", S("abcde"), false, true);
		test("", S("abcdefghij"), false, true);
		test("", S("abcdefghijklmnopqrst"), false, true);
		test("abcde", S(""), true, false);
		test("abcde", S("abcde"), true, true);
		test("abcde", S("abcdefghij"), false, true);
		test("abcde", S("abcdefghijklmnopqrst"), false, true);
		test("abcdefghij", S(""), true, false);
		test("abcdefghij", S("abcde"), true, false);
		test("abcdefghij", S("abcdefghij"), true, true);
		test("abcdefghij", S("abcdefghijklmnopqrst"), false, true);
		test("abcdefghijklmnopqrst", S(""), true, false);
		test("abcdefghijklmnopqrst", S("abcde"), true, false);
		test("abcdefghijklmnopqrst", S("abcdefghij"), true, false);
		test("abcdefghijklmnopqrst", S("abcdefghijklmnopqrst"), true,
		     true);
	}

#ifdef XXX // XXX: Implement constexpr_char_traits
	{
		typedef std::basic_string_view<char,
					       constexpr_char_traits<char>>
			SV;
		constexpr SV sv1;
		constexpr SV sv2{"abcde", 5};

		static_assert(sv1 >= "", "");
		static_assert("" >= sv1, "");
		static_assert(!(sv1 >= "abcde"), "");
		static_assert("abcde" >= sv1, "");

		static_assert(sv2 >= "", "");
		static_assert(!("" >= sv2), "");
		static_assert(sv2 >= "abcde", "");
		static_assert("abcde" >= sv2, "");
		static_assert(!(sv2 >= "abcde0"), "");
		static_assert("abcde0" >= sv2, "");
	}
#endif
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(argc, argv); });
}
