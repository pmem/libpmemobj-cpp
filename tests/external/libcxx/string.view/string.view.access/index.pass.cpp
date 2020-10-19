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

// constexpr const _CharT& operator[](size_type _pos) const;

#include "unittest.hpp"

#include <type_traits>

#include <libpmemobj++/string_view.hpp>

template <typename CharT>
void
test(const CharT *s, size_t len)
{
	typedef pmem::obj::basic_string_view<CharT> SV;
	SV sv(s, len);
	static_assert(std::is_same<decltype(sv[0]),
				   typename SV::const_reference>::value,
		      "must be const_reference");
	UT_ASSERT(noexcept(sv[0]) == true);
	UT_ASSERT(sv.length() == len);
	for (size_t i = 0; i < len; ++i) {
		UT_ASSERT(sv[i] == s[i]);
		UT_ASSERT(&sv[i] == s + i);
	}
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
		constexpr pmem::obj::basic_string_view<char> sv("ABC", 2);
		static_assert(sv.length() == 2, "");
		static_assert(sv[0] == 'A', "");
		static_assert(sv[1] == 'B', "");
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
