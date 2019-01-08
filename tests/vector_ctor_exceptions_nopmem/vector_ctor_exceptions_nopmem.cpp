/*
 * Copyright 2018-2019, Intel Corporation
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

#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/pool.hpp>

#include <cstring>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using vector_type = pmem_exp::vector<int>;

/**
 * Test pmem::obj::experimental::vector default constructor.
 *
 * Call default constructor for volatile instance of
 * pmem::obj::experimental::vector. Expect pmem::pool_error exception is thrown.
 */
void
test_default_ctor()
{
	bool exception_thrown = false;
	try {
		vector_type v = {};
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::experimental::vector range constructor.
 *
 * Call range constructor for volatile instance of
 * pmem::obj::experimental::vector. Expect pmem::pool_error exception is thrown.
 */
void
test_iter_iter_ctor()
{
	int a[] = {0, 1, 2, 3, 4, 5};

	bool exception_thrown = false;
	try {
		vector_type v(std::begin(a), std::end(a));
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::experimental::vector fill constructor with elements with
 * default values.
 *
 * Call fill constructor for volatile instance of
 * pmem::obj::experimental::vector. Expect pmem::pool_error exception is thrown.
 */
void
test_size_ctor()
{
	bool exception_thrown = false;
	try {
		vector_type v(100);
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::experimental::vector fill constructor with elements with
 * custom values.
 *
 * Call fill constructor for volatile instance of
 * pmem::obj::experimental::vector. Expect pmem::pool_error exception is thrown.
 */
void
test_size_value_ctor()
{
	bool exception_thrown = false;
	try {
		vector_type v(100, 5);
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

int
main(int argc, char *argv[])
{
	START();

	test_default_ctor();
	test_iter_iter_ctor();
	test_size_ctor();
	test_size_value_ctor();

	return 0;
}
