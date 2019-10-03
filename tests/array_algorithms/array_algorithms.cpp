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

#include <algorithm>
#include <iterator>

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct TestSort {
#ifdef NO_GCC_AGGREGATE_INITIALIZATION_BUG
	pmem::obj::array<double, 10> c = {10, 9, 8, 7, 6, 5, 4, 3, 2, 1};
#else
	pmem::obj::array<double, 10> c = {{10, 9, 8, 7, 6, 5, 4, 3, 2, 1}};
#endif
	void
	sort_single_element_snapshot()
	{
		std::sort(c.begin(), c.end());

		pmem::obj::array<double, 10> expected = {1, 2, 3, 4, 5,
							 6, 7, 8, 9, 10};

		UT_ASSERT(c == expected);
	}

	void
	sort_range_snapshot()
	{
		auto slice = c.range(0, c.size(), 2);

		std::sort(slice.begin(), slice.end());

		pmem::obj::array<double, 10> expected = {1, 2, 3, 4, 5,
							 6, 7, 8, 9, 10};

		UT_ASSERT(c == expected);
	}
};

struct root {
	pmem::obj::persistent_ptr<TestSort> test_sort;
};

void
test_sort_single_element(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test_sort = pmem::obj::make_persistent<TestSort>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test_sort->sort_single_element_snapshot();

			pmem::obj::transaction::abort(0);
			UT_ASSERT(0);
		});
	} catch (pmem::manual_tx_abort &) {
		pmem::obj::array<double, 10> expected = {10, 9, 8, 7, 6,
							 5,  4, 3, 2, 1};
		UT_ASSERT(r->test_sort->c == expected);
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<TestSort>(r->test_sort);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
test_sort_range(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test_sort = pmem::obj::make_persistent<TestSort>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->test_sort->sort_range_snapshot();

			pmem::obj::transaction::abort(0);
			UT_ASSERT(0);
		});
	} catch (pmem::manual_tx_abort &) {
		pmem::obj::array<double, 10> expected = {10, 9, 8, 7, 6,
							 5,  4, 3, 2, 1};
		UT_ASSERT(r->test_sort->c == expected);
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<TestSort>(r->test_sort);
		});
	} catch (...) {
		UT_ASSERT(0);
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

	test_sort_single_element(pop);
	test_sort_range(pop);

	pop.close();

	return 0;
}
