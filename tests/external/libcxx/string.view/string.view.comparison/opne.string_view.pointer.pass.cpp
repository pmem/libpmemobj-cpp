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

// template<class charT, class traits>
//   constexpr bool operator!=(basic_string_view<charT,traits> lhs, const charT*
//   rhs);
// template<class charT, class traits>
//   constexpr bool operator!=(const charT* lhs, basic_string_view<charT,traits>
//   rhs);

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(S lhs, const typename S::value_type *rhs, bool x)
{
	UT_ASSERT((lhs != rhs) == x);
	UT_ASSERT((rhs != lhs) == x);
}

static void
run(int argc, char *argv[])
{
	{
		typedef pmem::obj::string_view S;
		test(S(""), "", false);
		test(S(""), "abcde", true);
		test(S(""), "abcdefghij", true);
		test(S(""), "abcdefghijklmnopqrst", true);
		test(S("abcde"), "", true);
		test(S("abcde"), "abcde", false);
		test(S("abcde"), "abcdefghij", true);
		test(S("abcde"), "abcdefghijklmnopqrst", true);
		test(S("abcdefghij"), "", true);
		test(S("abcdefghij"), "abcde", true);
		test(S("abcdefghij"), "abcdefghij", false);
		test(S("abcdefghij"), "abcdefghijklmnopqrst", true);
		test(S("abcdefghijklmnopqrst"), "", true);
		test(S("abcdefghijklmnopqrst"), "abcde", true);
		test(S("abcdefghijklmnopqrst"), "abcdefghij", true);
		test(S("abcdefghijklmnopqrst"), "abcdefghijklmnopqrst", false);
	}

#ifdef XXX // XXX: Implement constexpr_char_traits
	{
		typedef std::basic_string_view<char,
					       constexpr_char_traits<char>>
			SV;
		constexpr SV sv1;
		constexpr SV sv2{"abcde", 5};

		static_assert(!(sv1 != ""), "");
		static_assert(!("" != sv1), "");
		static_assert(sv1 != "abcde", "");
		static_assert("abcde" != sv1, "");

		static_assert(!(sv2 != "abcde"), "");
		static_assert(!("abcde" != sv2), "");
		static_assert(sv2 != "abcde0", "");
		static_assert("abcde0" != sv2, "");
	}
#endif
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(argc, argv); });
}
