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

#include "iterators_support.hpp"
#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

#include <vector>

namespace nvobj = pmem::obj;

using C = container_t<int>;

struct root {
	nvobj::persistent_ptr<C> v;
};

void
check_vector(nvobj::pool<struct root> &pop, size_t count, int value)
{
	auto r = pop.root();

	UT_ASSERT(r->v->capacity() == expected_capacity(count));
	UT_ASSERT(r->v->size() == count);

	for (unsigned i = 0; i < count; ++i) {
		UT_ASSERT((*r->v)[i] == value);
	}
}

/**
 * Test pmem::obj::vector assign() methods
 *
 * Replace content of the vector with content greater than max_size()
 * Expect std::length_error exception is thrown
 * Methods under test:
 * - fill version of assign()
 * - range version of assign()
 */
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	check_vector(pop, 10, 1);

	auto size = r->v->max_size() + 1;

	/* assign() - fill version */
	bool exception_thrown = false;

	try {
		r->v->assign(size, 2);
		UT_ASSERT(0);
	} catch (std::length_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - range version */
	auto begin = test_support::counting_it<size_t>(0);
	/* we won't try to dereference this, it will be used for std::distance()
	 * calculation only */
	auto end = test_support::counting_it<size_t>(size);

	exception_thrown = false;

	try {
		r->v->assign(begin, end);
		UT_ASSERT(0);
	} catch (std::length_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);
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
		path, "VectorTest: vector_assign_exceptions_length",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v = nvobj::make_persistent<C>(10U, 1); });

		test(pop);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
