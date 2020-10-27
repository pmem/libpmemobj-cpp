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

// constexpr const _CharT& front();

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <typename CharT>
bool
test(const CharT *s, size_t len)
{
	typedef pmem::obj::basic_string_view<CharT> SV;
	SV sv(s, len);

	static_assert(std::is_same<decltype(sv.back()),
				   typename SV::const_reference>::value,
		      "must be const_reference");
#ifndef __cpp_lib_string_view
	static_assert(noexcept(sv.back()), "Operation must be noexcept");
#endif
	UT_ASSERT(sv.length() == len);
	UT_ASSERT(sv.back() == s[len - 1]);
	return &sv.back() == s + len - 1;
}

static void
run()
{
	UT_ASSERT(test("ABCDE", 5));
	UT_ASSERT(test("a", 1));

	UT_ASSERT(test(L"ABCDE", 5));
	UT_ASSERT(test(L"a", 1));

	UT_ASSERT(test(u"ABCDE", 5));
	UT_ASSERT(test(u"a", 1));

	UT_ASSERT(test(U"ABCDE", 5));
	UT_ASSERT(test(U"a", 1));

	{
		constexpr pmem::obj::basic_string_view<char> sv("ABC", 2);
		static_assert(sv.length() == 2, "");
		static_assert(sv.back() == 'B', "");
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
