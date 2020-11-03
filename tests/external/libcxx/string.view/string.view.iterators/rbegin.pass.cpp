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

// const_iterator rbegin() const;

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <class S>
void
test(void)
{
	S s;
	const S &cs = s;
	typename S::reverse_iterator b = s.rbegin();
	typename S::const_reverse_iterator cb1 = cs.rbegin();
	typename S::const_reverse_iterator cb2 = s.crbegin();

	UT_ASSERT(b == cb1);
	UT_ASSERT(b == cb2);
	UT_ASSERT(cb1 == cb2);
}

template <class S>
void
test(S s)
{
	const S &cs = s;
	typename S::reverse_iterator b = s.rbegin();
	typename S::const_reverse_iterator cb1 = cs.rbegin();
	typename S::const_reverse_iterator cb2 = s.crbegin();
	if (!s.empty()) {
		// volatile below will avoid aggressive optimisations under GCC
		volatile size_t last = s.size() - 1;
		UT_ASSERT(*b == s[last]);
		UT_ASSERT(&*b == &s[last]);
		UT_ASSERT(*cb1 == s[last]);
		UT_ASSERT(&*cb1 == &s[last]);
		UT_ASSERT(*cb2 == s[last]);
		UT_ASSERT(&*cb2 == &s[last]);
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

	test<string_view>();
	test<u16string_view>();
	test<u32string_view>();
	test<wstring_view>();
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
