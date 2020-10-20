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

// [string.view.capacity], capacity
// constexpr size_type size()     const noexcept;
// constexpr size_type length()   const noexcept;
// constexpr size_type max_size() const noexcept;
// constexpr bool empty()         const noexcept;

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <typename SV>
void
test1()
{
	{
		constexpr SV sv1;
		static_assert(sv1.size() == 0, "");
		static_assert(sv1.empty(), "");
		static_assert(sv1.size() == sv1.length(), "");
		static_assert(sv1.max_size() > sv1.size(), "");
	}

	{
		SV sv1;
		static_assert(noexcept(sv1.size()),
			      "Operation must be noexcept");
		static_assert(noexcept(sv1.empty()),
			      "Operation must be noexcept");
		static_assert(noexcept(sv1.max_size()),
			      "Operation must be noexcept");
		static_assert(noexcept(sv1.length()),
			      "Operation must be noexcept");
		UT_ASSERT(sv1.size() == 0);
		UT_ASSERT(sv1.empty());
		UT_ASSERT(sv1.size() == sv1.length());
		UT_ASSERT(sv1.max_size() > sv1.size());
	}
}

template <typename CharT>
void
test2(const CharT *s, size_t len)
{
	{
		pmem::obj::basic_string_view<CharT> sv1(s);
		UT_ASSERT(sv1.size() == len);
		UT_ASSERT(sv1.data() == s);
		UT_ASSERT(sv1.empty() == (len == 0));
		UT_ASSERT(sv1.size() == sv1.length());
		UT_ASSERT(sv1.max_size() > sv1.size());
	}
}

static void
run()
{
	test1<pmem::obj::string_view>();
	test1<pmem::obj::u16string_view>();
	test1<pmem::obj::u32string_view>();
	test1<pmem::obj::wstring_view>();

	test2("ABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDE",
	      105);
	test2("ABCDE", 5);
	test2("a", 1);
	test2("", 0);

	test2(L"ABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDE",
	      105);
	test2(L"ABCDE", 5);
	test2(L"a", 1);
	test2(L"", 0);

	test2(u"ABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDE",
	      105);
	test2(u"ABCDE", 5);
	test2(u"a", 1);
	test2(u"", 0);

	test2(U"ABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDEABCDE",
	      105);
	test2(U"ABCDE", 5);
	test2(U"a", 1);
	test2(U"", 0);
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
