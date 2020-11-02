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

// constexpr basic_string_view substr(size_type pos = 0, size_type n = npos)
// const;

// Throws: out_of_range if pos > size().
// Effects: Determines the effective length rlen of the string to reference as
// the smaller of n and size() - pos. Returns: basic_string_view(data()+pos,
// rlen).

#include "unittest.hpp"

#include <algorithm>
#include <libpmemobj++/string_view.hpp>
#include <stdexcept>

template <typename CharT>
void
test1(pmem::obj::basic_string_view<CharT> sv, size_t n, size_t pos)
{
	pmem::obj::basic_string_view<CharT> sv1;
	try {
		sv1 = sv.substr(pos, n);
		UT_ASSERT(pos <= sv.size());
	} catch (const std::out_of_range &) {
		UT_ASSERT(pos > sv.size());
		return;
	}

	const size_t rlen = (std::min)(n, sv.size() - pos);
	UT_ASSERT(sv1.size() == rlen);
	for (size_t i = 0; i < rlen; ++i)
		UT_ASSERT(sv[pos + i] == sv1[i]);
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

	test1(sv1, 0, 3);
	test1(sv1, 2, 3);
	test1(sv1, 100, 3);

	test1(sv1, 0, string_view_t::npos);
	test1(sv1, 2, string_view_t::npos);
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
