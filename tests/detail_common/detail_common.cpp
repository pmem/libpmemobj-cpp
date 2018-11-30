/*
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
main()
{
	START();

	test_next_pow_2();

	return 0;
}
