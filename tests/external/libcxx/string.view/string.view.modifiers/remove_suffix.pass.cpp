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

// void remove_suffix(size_type _n)

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <typename CharT>
void
test(const CharT *s, size_t len)
{
	typedef pmem::obj::basic_string_view<CharT> SV;
	{
		SV sv1(s);
		UT_ASSERT(sv1.size() == len);
		UT_ASSERT(sv1.data() == s);

		if (len > 0) {
			sv1.remove_suffix(1);
			UT_ASSERT(sv1.size() == (len - 1));
			UT_ASSERT(sv1.data() == s);
			sv1.remove_suffix(len - 1);
		}

		UT_ASSERT(sv1.size() == 0);
		sv1.remove_suffix(0);
		UT_ASSERT(sv1.size() == 0);
	}
}

static void
run()
{
	test("ABCDE", 5);
	test("a", 1);
	test("", 0);

	test(L"ABCDE", 5);
	test(L"a", 1);
	test(L"", 0);

	test(u"ABCDE", 5);
	test(u"a", 1);
	test(u"", 0);

	test(U"ABCDE", 5);
	test(U"a", 1);
	test(U"", 0);
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
