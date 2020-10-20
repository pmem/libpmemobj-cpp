//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <string_view>

// void swap(basic_string_view& _other) noexcept

#include "unittest.hpp"

#include <libpmemobj++/string_view.hpp>

template <typename CharT>
void
test(const CharT *s, size_t len)
{
	typedef pmem::obj::basic_string_view<CharT> SV;
	{
		SV sv1(s);
		SV sv2;

		UT_ASSERT(sv1.size() == len);
		UT_ASSERT(sv1.data() == s);
		UT_ASSERT(sv2.size() == 0);

		sv1.swap(sv2);
		UT_ASSERT(sv1.size() == 0);
		UT_ASSERT(sv2.size() == len);
		UT_ASSERT(sv2.data() == s);
	}
}

constexpr size_t
test_ce(size_t n, size_t k)
{
	typedef pmem::obj::basic_string_view<char> SV;
	SV sv1{"ABCDEFGHIJKL", n};
	SV sv2{sv1.data(), k};
	sv1.swap(sv2);
	return sv1.size();
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

	{
		static_assert(test_ce(2, 3) == 3, "");
		static_assert(test_ce(5, 3) == 3, "");
		static_assert(test_ce(0, 1) == 1, "");
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run(); });
}
