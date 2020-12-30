// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

#include <iterator>
#include <vector>

namespace nvobj = pmem::obj;

const static size_t pool_size = 2 * PMEMOBJ_MIN_POOL;
const static size_t test_val = pool_size * 2;

using vector_type = container_t<int>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr;
};

/**
 * Test pmem::obj::vector range constructor.
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
 * Test pmem::obj::vector fill constructor with elements with
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

	exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&] {
			pptr = nvobj::make_persistent<vector_type>(test_val);
		});
		UT_ASSERT(0);
	} catch (pmem::transaction_out_of_memory &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::vector fill constructor with elements with
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

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
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
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
