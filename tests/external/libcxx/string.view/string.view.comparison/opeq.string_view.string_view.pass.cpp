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
//   constexpr bool operator==(const basic_string_view<charT,traits> lhs,
//                   const basic_string_view<charT,traits> rhs);

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(S lhs, S rhs, bool x)
{
	UT_ASSERT((lhs == rhs) == x);
	UT_ASSERT((rhs == lhs) == x);
}

static void
run(int argc, char *argv[])
{
	{
		typedef pmem::obj::string_view S;
		test(S(""), S(""), true);
		test(S(""), S("abcde"), false);
		test(S(""), S("abcdefghij"), false);
		test(S(""), S("abcdefghijklmnopqrst"), false);
		test(S("abcde"), S(""), false);
		test(S("abcde"), S("abcde"), true);
		test(S("abcde"), S("abcdefghij"), false);
		test(S("abcde"), S("abcdefghijklmnopqrst"), false);
		test(S("abcdefghij"), S(""), false);
		test(S("abcdefghij"), S("abcde"), false);
		test(S("abcdefghij"), S("abcdefghij"), true);
		test(S("abcdefghij"), S("abcdefghijklmnopqrst"), false);
		test(S("abcdefghijklmnopqrst"), S(""), false);
		test(S("abcdefghijklmnopqrst"), S("abcde"), false);
		test(S("abcdefghijklmnopqrst"), S("abcdefghij"), false);
		test(S("abcdefghijklmnopqrst"), S("abcdefghijklmnopqrst"),
		     true);
	}

#ifdef XXX // XXX: port constexpr_char_traits
	{
		typedef std::basic_string_view<char,
					       constexpr_char_traits<char>>
			SV;
		constexpr SV sv1;
		constexpr SV sv2;
		constexpr SV sv3{"abcde", 5};
		static_assert(sv1 == sv2, "");
		static_assert(!(sv1 == sv3), "");
	}
#endif
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(argc, argv); });
}
