// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "iterators_support.hpp"
#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

#include <vector>

namespace nvobj = pmem::obj;

using C = container_t<size_t>;

struct root {
	nvobj::persistent_ptr<C> v;
};

void
check_vector(nvobj::pool<struct root> &pop, size_t count, size_t value)
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

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: vector_assign_exceptions_length",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v = nvobj::make_persistent<C>(10U, 1U);
		});

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
