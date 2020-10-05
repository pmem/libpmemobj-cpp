//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <string>

// template<class charT, class traits, class Allocator>
//   bool operator>=(const basic_string<charT,traits,Allocator>& lhs,
//                   basic_string_view<charT,traits> rhs);
//   bool operator>=(basic_string_view<charT,traits> lhs,
//            const basic_string<charT,traits,Allocator>&  rhs);

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(const S &lhs, const typename S::value_type *rhs, bool x, bool y)
{
	UT_ASSERT((lhs >= rhs) == x);
	UT_ASSERT((rhs >= lhs) == y);
}

static void
run(int argc, char *argv[])
{
	{
		typedef pmem::obj::string_view S;
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

int
main(int argc, char *argv[])
{
	return run_test([&] { run(argc, argv); });
}
