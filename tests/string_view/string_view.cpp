// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <iostream>
#include <libpmemobj++/string_view.hpp>

void
test_string_view()
{
	std::string s1("abc");
	std::string s2("xyz");

	pmem::obj::string_view v1(s1);
	pmem::obj::string_view v2(s2);

	UT_ASSERT(s1.data() == v1.data());
	UT_ASSERT(s1.size() == v1.size());

	UT_ASSERT(v1.compare(v2) < 0);
	UT_ASSERT(v2.compare(v1) > 0);
	UT_ASSERT(v1.compare(v1) == 0);
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test_string_view(); });
}
