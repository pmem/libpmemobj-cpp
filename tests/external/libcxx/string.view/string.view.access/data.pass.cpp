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

// constexpr const _CharT* data() const noexcept;

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <typename CharT>
void
test(const CharT *s, size_t len)
{
	pmem::obj::basic_string_view<CharT> sv(s, len);
	UT_ASSERT(sv.length() == len);
	UT_ASSERT(sv.data() == s);
}

static void
run()
{
	test("ABCDE", 5);
	test("a", 1);

	test(L"ABCDE", 5);
	test(L"a", 1);

	test(u"ABCDE", 5);
	test(u"a", 1);

	test(U"ABCDE", 5);
	test(U"a", 1);

	{
		constexpr const char *s = "ABC";
		constexpr pmem::obj::basic_string_view<char> sv(s, 2);
		static_assert(sv.length() == 2, "");
		static_assert(sv.data() == s, "");
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
