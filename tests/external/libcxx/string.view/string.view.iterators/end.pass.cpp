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

// constexpr const_iterator end() const;

#include "unittest.hpp"

#include <cstddef>
#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(S s)
{
	const S &cs = s;
	typename S::iterator e = s.end();
	typename S::const_iterator ce1 = cs.end();
	typename S::const_iterator ce2 = s.cend();

	if (s.empty()) {
		UT_ASSERT(e == s.begin());
		UT_ASSERT(ce1 == cs.begin());
		UT_ASSERT(ce2 == s.begin());
	} else {
		UT_ASSERT(e != s.begin());
		UT_ASSERT(ce1 != cs.begin());
		UT_ASSERT(ce2 != s.begin());
	}

	UT_ASSERT(static_cast<std::size_t>(e - s.begin()) == s.size());
	UT_ASSERT(static_cast<std::size_t>(ce1 - cs.begin()) == cs.size());
	UT_ASSERT(static_cast<std::size_t>(ce2 - s.cbegin()) == s.size());

	UT_ASSERT(e == ce1);
	UT_ASSERT(e == ce2);
	UT_ASSERT(ce1 == ce2);
}

static void
run()
{
	typedef pmem::obj::string_view string_view;
	typedef pmem::obj::u16string_view u16string_view;
	typedef pmem::obj::u32string_view u32string_view;
	typedef pmem::obj::wstring_view wstring_view;

	test(string_view());
	test(u16string_view());
	test(u32string_view());
	test(wstring_view());
	test(string_view("123"));
	test(wstring_view(L"123"));
	test(u16string_view{u"123"});
	test(u32string_view{U"123"});

	{
		constexpr string_view sv{"123", 3};
		constexpr u16string_view u16sv{u"123", 3};
		constexpr u32string_view u32sv{U"123", 3};
		constexpr wstring_view wsv{L"123", 3};

		static_assert(sv.begin() != sv.end(), "");
		static_assert(u16sv.begin() != u16sv.end(), "");
		static_assert(u32sv.begin() != u32sv.end(), "");
		static_assert(wsv.begin() != wsv.end(), "");

		static_assert(sv.begin() != sv.cend(), "");
		static_assert(u16sv.begin() != u16sv.cend(), "");
		static_assert(u32sv.begin() != u32sv.cend(), "");
		static_assert(wsv.begin() != wsv.cend(), "");
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
