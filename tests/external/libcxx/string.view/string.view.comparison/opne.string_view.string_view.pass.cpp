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

// <string_view>

// template<class charT, class traits, class Allocator>
//   constexpr bool operator!=(const basic_string_view<charT,traits> lhs,
//                   const basic_string_view<charT,traits> rhs);

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(S lhs, S rhs, bool x)
{
	UT_ASSERT((lhs != rhs) == x);
	UT_ASSERT((rhs != lhs) == x);
}

static void
run(int argc, char *argv[])
{
	{
		typedef pmem::obj::string_view S;
		test(S(""), S(""), false);
		test(S(""), S("abcde"), true);
		test(S(""), S("abcdefghij"), true);
		test(S(""), S("abcdefghijklmnopqrst"), true);
		test(S("abcde"), S(""), true);
		test(S("abcde"), S("abcde"), false);
		test(S("abcde"), S("abcdefghij"), true);
		test(S("abcde"), S("abcdefghijklmnopqrst"), true);
		test(S("abcdefghij"), S(""), true);
		test(S("abcdefghij"), S("abcde"), true);
		test(S("abcdefghij"), S("abcdefghij"), false);
		test(S("abcdefghij"), S("abcdefghijklmnopqrst"), true);
		test(S("abcdefghijklmnopqrst"), S(""), true);
		test(S("abcdefghijklmnopqrst"), S("abcde"), true);
		test(S("abcdefghijklmnopqrst"), S("abcdefghij"), true);
		test(S("abcdefghijklmnopqrst"), S("abcdefghijklmnopqrst"),
		     false);
	}

#ifdef XXX // XXX: Implement constexpr_char_traits
	{
		typedef std::basic_string_view<char,
					       constexpr_char_traits<char>>
			SV;
		constexpr SV sv1;
		constexpr SV sv2;
		constexpr SV sv3{"abcde", 5};
		static_assert(!(sv1 != sv2), "");
		static_assert(sv1 != sv3, "");
	}
#endif
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(argc, argv); });
}
