// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

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

	UT_ASSERTeq(r->v->size(), count);

	for (unsigned i = 0; i < count; ++i) {
		UT_ASSERTeq((*r->v)[i], value);
	}
}

/**
 * Test pmem::obj::vector assign() methods
 *
 * Checks if vector's state is reverted when transaction aborts.
 * Methods under test:
 * - fill version of assign()
 * - range version of assign()
 * - initializer list version of assign()
 * - copy assignment operator
 * - move assignment operator
 * - initializer list assignment operator
 */
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	check_vector(pop, 10, 1);

	/* assign() - fill version */
	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			r->v->assign(100, 2);
			check_vector(pop, 100, 2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - range version */
	exception_thrown = false;
	std::vector<int> v2(100, 2);
	try {
		nvobj::transaction::run(pop, [&] {
			r->v->assign(v2.begin(), v2.end());
			check_vector(pop, 100, 2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - initializer list version */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			r->v->assign({2, 2, 2, 2, 2});
			check_vector(pop, 5, 2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - copy version */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			r->v->assign(*v2);
			check_vector(pop, 100, 2);
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - move version */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			r->v->assign(std::move(*v2));
			check_vector(pop, 100, 2);
			UT_ASSERT(v2->empty());
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* copy assignment operator */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			*r->v = *v2;
			check_vector(pop, 100, 2);
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* move assignment operator */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			*r->v = std::move(*v2);
			check_vector(pop, 100, 2);
			UT_ASSERT(v2->empty());
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* initializer list assignment operator */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			*r->v = {2, 2, 2, 2, 2};
			check_vector(pop, 5, 2);
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
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
	auto pop = nvobj::pool<root>::create(path, "VectorTest: assign_txabort",
					     2 * PMEMOBJ_MIN_POOL,
					     S_IWUSR | S_IRUSR);

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
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
