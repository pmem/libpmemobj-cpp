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
 * Test pmem::obj::vector capacity methods
 *
 * Checks if vector's state is reverted when transaction aborts.
 * Methods under test:
 * - reserve()
 * - shrink_to_fit()
 */
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	UT_ASSERT(r->v->capacity() == expected_capacity(100U));

	bool exception_thrown = false;

	/* test reserve() revert */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v->reserve(150);
			UT_ASSERT(r->v->capacity() == expected_capacity(150U));
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v->capacity() == expected_capacity(100U));
	UT_ASSERT(r->v->size() == 100);
	for (unsigned i = 0; i < r->v->size(); ++i)
		UT_ASSERT(r->v->const_at(i) == 0);
	UT_ASSERT(exception_thrown);

	/* test shrink_to_fit() revert */
	try {
		r->v->reserve(150);
		UT_ASSERT(r->v->capacity() == expected_capacity(150U));

		nvobj::transaction::run(pop, [&] {
			r->v->shrink_to_fit();
			UT_ASSERT(r->v->capacity() == expected_capacity(100U));
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v->capacity() == expected_capacity(150U));
	UT_ASSERT(r->v->size() == 100);
	for (unsigned i = 0; i < r->v->size(); ++i)
		UT_ASSERT(r->v->const_at(i) == 0);
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
		path, "VectorTest: capacity_txabort", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

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
