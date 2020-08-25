// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <iostream>
#include <libpmemobj++/string_view.hpp>
#include <string>

template <typename T>
void
test_string_view()
{
	std::string s1("abc");
	std::string s2("xyz");
	std::string sLonger("01234567890123456789"
			    "01234567890123456789"
			    "01234567890123456789"
			    "01234567890123456789"
			    "01234567890123456789");
	std::string sLonger2("xyz34567890123456789"
			     "01234567890123456789"
			     "01234567890123456789"
			     "01234567890123456789"
			     "01234567890123456789");

	std::basic_string<T> ts1(s1.begin(), s1.end());
	std::basic_string<T> ts2(s2.begin(), s2.end());
	std::basic_string<T> tsLonger(sLonger.begin(), sLonger.end());
	std::basic_string<T> tsLonger2(sLonger2.begin(), sLonger2.end());

	pmem::obj::basic_string_view<T> vEmpty;
	pmem::obj::basic_string_view<T> v1(ts1.data(), ts1.length());
	pmem::obj::basic_string_view<T> v2(ts2.data(), ts2.length());
	pmem::obj::basic_string_view<T> vLonger(tsLonger.data(),
						tsLonger.length());
	pmem::obj::basic_string_view<T> vLonger2(tsLonger2.data(),
						 tsLonger2.length());

	UT_ASSERT(ts1.data() == v1.data());
	UT_ASSERT(ts1.size() == v1.size());

	UT_ASSERT(v1.compare(v2) < 0);
	UT_ASSERT(v2.compare(v1) > 0);
	UT_ASSERT(v1.compare(v1) == 0);

	UT_ASSERT(v1.compare(vLonger) != 0);

	UT_ASSERT(v2.compare(vLonger2) < 0);
	UT_ASSERT(vLonger2.compare(v2) > 0);

	UT_ASSERT(vEmpty.data() == nullptr);
	UT_ASSERT(vEmpty.size() == 0);
}

void
run_test()
{
	test_string_view<char>();
	test_string_view<wchar_t>();
	test_string_view<uint8_t>();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { run_test(); });
}
