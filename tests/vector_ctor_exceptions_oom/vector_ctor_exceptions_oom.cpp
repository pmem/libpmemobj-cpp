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
#include <libpmemobj++/make_persistent.hpp>

#include <iterator>
#include <vector>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

const static size_t pool_size = 2 * PMEMOBJ_MIN_POOL;
const static size_t test_val = pool_size * 2;

using vector_type = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr;
};

/**
 * Test pmem::obj::experimental::vector range constructor.
 *
 * Call range constructor to exceed available memory of the pool. Expect
 * pmem:transaction_alloc_error exception is thrown.
 */
void
test_iter_iter_ctor(nvobj::pool<struct root> &pop,
		    nvobj::persistent_ptr<vector_type> &pptr)
{
	static std::vector<int> vec(test_val);

	bool exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&] {
			pptr = nvobj::make_persistent<vector_type>(
				std::begin(vec), std::end(vec));
		});
		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
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
 * Call fill constructor to exceed available memory of the pool. Expect
 * pmem:transaction_alloc_error exception is thrown.
 */
void
test_size_ctor(nvobj::pool<struct root> &pop,
	       nvobj::persistent_ptr<vector_type> &pptr)
{
	bool exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&] {
			pptr = nvobj::make_persistent<vector_type>(test_val);
		});
		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
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
 * Call fill constructor to exceed available memory of the pool.
 * Expect pmem:transaction_alloc_error exception is thrown.
 */
void
test_size_value_ctor(nvobj::pool<struct root> &pop,
		     nvobj::persistent_ptr<vector_type> &pptr)
{
	bool exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&] {
			pptr = nvobj::make_persistent<vector_type>(test_val, 1);
		});
		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
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

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: vector_ctor_exceptions_oom", pool_size,
		S_IWUSR | S_IRUSR);

	auto pptr = pop.root()->pptr;

	test_iter_iter_ctor(pop, pptr);
	test_size_ctor(pop, pptr);
	test_size_value_ctor(pop, pptr);
	/* XXX: implement following test cases when vector's push_back method is
	   available */
	// test_copy_ctor(pop);
	// test_initializer_list_ctor(pop);

	pop.close();

	return 0;
}
