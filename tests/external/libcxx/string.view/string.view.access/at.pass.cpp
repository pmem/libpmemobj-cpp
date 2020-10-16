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

// NOTE: Older versions of clang have a bug where they fail to evaluate
// string_view::at as a constant expression.
// XFAIL: clang-3.4, clang-3.3

// <string_view>

// constexpr const _CharT& at(size_type _pos) const;

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <typename CharT>
void
test(const CharT *s, size_t len)
{
	pmem::obj::basic_string_view<CharT> sv(s, len);
	UT_ASSERT(sv.length() == len);
	for (size_t i = 0; i < len; ++i) {
		UT_ASSERT(sv.at(i) == s[i]);
		UT_ASSERT(&sv.at(i) == s + i);
	}

	try {
		(void)sv.at(len);
	} catch (const std::out_of_range &) {
		return;
	}
	UT_ASSERT(false);
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
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
