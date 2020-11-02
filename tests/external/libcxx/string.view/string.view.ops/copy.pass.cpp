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

// size_type copy(charT* s, size_type n, size_type pos = 0) const;

// Throws: out_of_range if pos > size().
// Remarks: Let rlen be the smaller of n and size() - pos.
// Requires: [s, s+rlen) is a valid range.
// Effects: Equivalent to std::copy_n(begin() + pos, rlen, s).
// Returns: rlen.

#include "unittest.hpp"

#include <algorithm>
#include <libpmemobj++/string_view.hpp>
#include <stdexcept>

template <typename CharT>
void
test1(pmem::obj::basic_string_view<CharT> sv, size_t n, size_t pos)
{
	const size_t rlen = (std::min)(n, sv.size() - pos);

	CharT *dest1 = new CharT[rlen + 1];
	dest1[rlen] = 0;
	CharT *dest2 = new CharT[rlen + 1];
	dest2[rlen] = 0;

	if (pos > sv.size()) {
		try {
			sv.copy(dest1, n, pos);
			UT_ASSERT(false);
		} catch (const std::out_of_range &) {
		} catch (...) {
			UT_ASSERT(false);
		}
	} else {
		UT_ASSERT(sv.copy(dest1, n, pos) == rlen);
		std::copy_n(sv.begin() + pos, rlen, dest2);
		for (size_t i = 0; i <= rlen; ++i)
			UT_ASSERT(dest1[i] == dest2[i]);
	}
	delete[] dest1;
	delete[] dest2;
}

template <typename CharT>
void
test(const CharT *s)
{
	typedef pmem::obj::basic_string_view<CharT> string_view_t;

	string_view_t sv1(s);

	test1(sv1, 0, 0);
	test1(sv1, 1, 0);
	test1(sv1, 20, 0);
	test1(sv1, sv1.size(), 0);
	test1(sv1, 20, string_view_t::npos);

	test1(sv1, 0, 3);
	test1(sv1, 2, 3);
	test1(sv1, 100, 3);
	test1(sv1, 100, string_view_t::npos);

	test1(sv1, sv1.size(), string_view_t::npos);

	test1(sv1, sv1.size() + 1, 0);
	test1(sv1, sv1.size() + 1, 1);
	test1(sv1, sv1.size() + 1, string_view_t::npos);
}

static void
run()
{
	test("ABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDE");
	test("ABCDE");
	test("a");
	test("");

	test(L"ABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDE");
	test(L"ABCDE");
	test(L"a");
	test(L"");

	test(u"ABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDE");
	test(u"ABCDE");
	test(u"a");
	test(u"");

	test(U"ABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDE");
	test(U"ABCDE");
	test(U"a");
	test(U"");
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
