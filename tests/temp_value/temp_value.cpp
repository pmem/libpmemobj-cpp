/*
 * Copyright 2019, Intel Corporation
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
 * temp_value.c -- temp_value caching struct test
 */

#include "unittest.hpp"

#include <libpmemobj++/detail/temp_value.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj.h>

namespace nvobj = pmem::obj;
namespace det = pmem::detail;

const unsigned big_stack_alloc =
	LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE / sizeof(int) + 1;

struct root {
};

struct test_small {
	test_small() noexcept {};

	test_small(int a){}; /* may throw */
};

struct test_big {
	int a[big_stack_alloc];

	test_big() noexcept {};

	test_big(int a){}; /* may throw */
};

using temp_noexcept_small = det::temp_value<test_small, noexcept(test_small())>;
using temp_throw_small = det::temp_value<test_small, noexcept(test_small(1))>;
using temp_noexcept_big = det::temp_value<test_big, noexcept(test_big())>;
using temp_throw_big = det::temp_value<test_big, noexcept(test_big(1))>;

template <typename T>
bool
is_pmem(T &ptr)
{
	return nullptr != pmemobj_pool_by_ptr(static_cast<void *>(&ptr));
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<struct root>::create(path, "temp_value test",
						       PMEMOBJ_MIN_POOL,
						       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			temp_noexcept_small tmp1;
			UT_ASSERT(!is_pmem<test_small>(tmp1.get()));

			temp_throw_small tmp2;
			UT_ASSERT(is_pmem<test_small>(tmp2.get()));

			temp_noexcept_big tmp3;
			UT_ASSERT(is_pmem<test_big>(tmp3.get()));

			temp_throw_big tmp4;
			UT_ASSERT(is_pmem<test_big>(tmp3.get()));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
