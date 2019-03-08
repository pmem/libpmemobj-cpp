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

#include "unittest.hpp"

#include <algorithm>
#include <iterator>

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmemobj_exp = pmem::obj::experimental;

using array_type = pmemobj_exp::array<double, 5>;

struct root {
	pmem::obj::persistent_ptr<array_type> ptr_a;
	pmem::obj::persistent_ptr<array_type> ptr_b;
};

void
test_modifiers(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_a = pmem::obj::make_persistent<array_type>();
			r->ptr_b = pmem::obj::make_persistent<array_type>();
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	r->ptr_a->fill(2.4);
	r->ptr_b->fill(1.0);

	r->ptr_a->swap(*(r->ptr_b));

	*(r->ptr_a) = *(r->ptr_b);
	*(r->ptr_b) = std::move(*(r->ptr_a));

	*(r->ptr_a) = *(r->ptr_b);

	*(r->ptr_b) = *(r->ptr_b);
	UT_ASSERT(*(r->ptr_a) == *(r->ptr_b));

	*(r->ptr_b) = std::move(*(r->ptr_b));
	UT_ASSERT(*(r->ptr_a) == *(r->ptr_b));

	r->ptr_b->swap(*(r->ptr_b));
	UT_ASSERT(*(r->ptr_a) == *(r->ptr_b));

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<array_type>(r->ptr_a);
			pmem::obj::delete_persistent<array_type>(r->ptr_b);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/*
	 * All of following tests should fail - calling any array 'modifier'
	 * on object which is not on pmem should throw exception.
	 */
	array_type stack_array;

	try {
		stack_array.fill(1.0);
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		stack_array.swap(*(r->ptr_a));
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		stack_array = *(r->ptr_a);
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		stack_array = std::move(*(r->ptr_a));
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		stack_array.swap(stack_array);
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		/* Workaround for -Wself-assign-overloaded compile error */
		auto &ref = stack_array;

		stack_array = ref;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		array_type &ref = stack_array;

		stack_array = std::move(ref);
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
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

	auto pop = pmem::obj::pool<root>::create(
		path, "ArrayTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_modifiers(pop);

	pop.close();

	return 0;
}
