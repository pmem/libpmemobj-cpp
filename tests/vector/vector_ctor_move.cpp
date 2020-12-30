// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using vector_type = container_t<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v1;
	nvobj::persistent_ptr<vector_type> v2;
};

/**
 * Test pmem::obj::vector move constructor.
 *
 * Checks if vector state is reverted when transaction aborts
 */
void
test_move_ctor_abort(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	auto size = r->v1->size();

	UT_ASSERT(r->v2 == nullptr);
	try {
		nvobj::transaction::run(pop, [&] {
			r->v2 = nvobj::make_persistent<vector_type>(
				std::move(*r->v1));

			UT_ASSERT(r->v1->empty());
			UT_ASSERT(r->v2->size() == size);

			for (vector_type::size_type i = 0; i < size; ++i) {
				UT_ASSERT((*r->v2)[i] == (int)i);
			}

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v2 == nullptr);
	UT_ASSERT(r->v1->size() == size);

	try {
		nvobj::transaction::run(pop, [&] {
			for (vector_type::size_type i = 0; i < size; ++i) {
				UT_ASSERT((*r->v1)[i] == (int)i);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: vector_ctor_move", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

	int arr[] = {0, 1, 2, 3, 4, 5};

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<vector_type>(
				std::begin(arr), std::end(arr));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	test_move_ctor_abort(pop);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<vector_type>(r->v1);
		});
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
