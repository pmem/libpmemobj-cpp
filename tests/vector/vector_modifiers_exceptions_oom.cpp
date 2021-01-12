// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using C = container_t<int>;

struct root {
	nvobj::persistent_ptr<C> v;
};

/**
 * Test pmem::obj::vector modifiers
 *
 * Increase size of the vector to value greater than pool size
 * Methods under test:
 * - resize()
 * - resize() with value
 * Expect pmem::transaction_allor_error exception is thrown
 */
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	UT_ASSERT(r->v->capacity() == expected_capacity<size_t>(100));
	UT_ASSERT(r->v->size() == 100);

	bool exception_thrown = false;

	auto size = r->v->max_size();

	/* test resize() */
	try {
		r->v->resize(size);
		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->v->capacity() == expected_capacity<size_t>(100));
	UT_ASSERT(r->v->size() == 100);

	/* test resize() with value */
	try {
		r->v->resize(size, 0);
		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->v->capacity() == expected_capacity<size_t>(100));
	UT_ASSERT(r->v->size() == 100);
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: modifiers_exceptions_oom",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v = nvobj::make_persistent<C>(100U); });

		test(pop);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
