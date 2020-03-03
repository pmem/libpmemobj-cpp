// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * detail_common.cpp -- detail/common.hpp functions tests
 */
#include "unittest.hpp"

#include <libpmemobj++/detail/common.hpp>

void
test_next_pow_2()
{
	UT_ASSERT(1 == pmem::detail::next_pow_2(0U));
	UT_ASSERT(1 == pmem::detail::next_pow_2(1U));
	UT_ASSERT(2 == pmem::detail::next_pow_2(2U));
	UT_ASSERT(4 == pmem::detail::next_pow_2(3U));
	UT_ASSERT(4096 == pmem::detail::next_pow_2(2507U));
	UT_ASSERT(1 == pmem::detail::next_pow_2(static_cast<uint64_t>(0)));
	UT_ASSERT(1 == pmem::detail::next_pow_2(static_cast<uint64_t>(1)));
	UT_ASSERT(2 == pmem::detail::next_pow_2(static_cast<uint64_t>(2)));
	UT_ASSERT(4 == pmem::detail::next_pow_2(static_cast<uint64_t>(3)));
	UT_ASSERT(4096 ==
		  pmem::detail::next_pow_2(static_cast<uint64_t>(2507)));
	UT_ASSERT(1ULL << 32 ==
		  pmem::detail::next_pow_2(static_cast<uint64_t>(1ULL << 32)));
	UT_ASSERT(1ULL << 33 ==
		  pmem::detail::next_pow_2(
			  static_cast<uint64_t>((1ULL << 32) + 1)));
	UT_ASSERT(1ULL << 50 ==
		  pmem::detail::next_pow_2(static_cast<uint64_t>(
			  (1ULL << 49) + (1ULL << 38) + (1ULL << 17) + 2507)));
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test_next_pow_2(); });
}
