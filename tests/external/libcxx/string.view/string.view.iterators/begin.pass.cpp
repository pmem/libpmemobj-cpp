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

// constexpr const_iterator begin() const;

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(S s)
{
	const S &cs = s;
	typename S::iterator b = s.begin();
	typename S::const_iterator cb1 = cs.begin();
	typename S::const_iterator cb2 = s.cbegin();
	if (!s.empty()) {
		UT_ASSERT(*b == s[0]);
		UT_ASSERT(&*b == &s[0]);
		UT_ASSERT(*cb1 == s[0]);
		UT_ASSERT(&*cb1 == &s[0]);
		UT_ASSERT(*cb2 == s[0]);
		UT_ASSERT(&*cb2 == &s[0]);
	}
	UT_ASSERT(b == cb1);
	UT_ASSERT(b == cb2);
	UT_ASSERT(cb1 == cb2);
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

		static_assert(*sv.begin() == sv[0], "");
		static_assert(*u16sv.begin() == u16sv[0], "");
		static_assert(*u32sv.begin() == u32sv[0], "");
		static_assert(*wsv.begin() == wsv[0], "");

		static_assert(*sv.cbegin() == sv[0], "");
		static_assert(*u16sv.cbegin() == u16sv[0], "");
		static_assert(*u32sv.cbegin() == u32sv[0], "");
		static_assert(*wsv.cbegin() == wsv[0], "");
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
