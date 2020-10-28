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

// constexpr const_iterator rend() const;

#include "unittest.hpp"

#include <cstddef>
#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(S s)
{
	const S &cs = s;
	typename S::reverse_iterator e = s.rend();
	typename S::const_reverse_iterator ce1 = cs.rend();
	typename S::const_reverse_iterator ce2 = s.crend();

	if (s.empty()) {
		UT_ASSERT(e == s.rbegin());
		UT_ASSERT(ce1 == cs.rbegin());
		UT_ASSERT(ce2 == s.rbegin());
	} else {
		UT_ASSERT(e != s.rbegin());
		UT_ASSERT(ce1 != cs.rbegin());
		UT_ASSERT(ce2 != s.rbegin());
	}

	UT_ASSERT(static_cast<std::size_t>(e - s.rbegin()) == s.size());
	UT_ASSERT(static_cast<std::size_t>(ce1 - cs.rbegin()) == cs.size());
	UT_ASSERT(static_cast<std::size_t>(ce2 - s.crbegin()) == s.size());

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
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
