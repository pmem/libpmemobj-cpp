// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "helper_classes.hpp"
#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

const static size_t pool_size = PMEMOBJ_MIN_POOL * 2;

using test_type = emplace_constructible_copy_insertable_move_insertable<int>;
using vector_type = container_t<test_type>;
using It = test_support::input_it<test_type *>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr1;
	nvobj::persistent_ptr<vector_type> pptr2;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: vector_ctor_check_copy", pool_size,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	/**
	 * Check if range ctor will construct vector from element's type
	 * copy ctor.
	 */
	test_type arr[] = {1, 2, 3, 4};

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr1 = nvobj::make_persistent<vector_type>(
				It(std::begin(arr)), It(std::end(arr)));
		});

		UT_ASSERTeq(r->pptr1->const_at(0).value, 1);
		UT_ASSERTeq(r->pptr1->const_at(1).value, 2);
		UT_ASSERTeq(r->pptr1->const_at(2).value, 3);
		UT_ASSERTeq(r->pptr1->const_at(3).value, 4);

		UT_ASSERTeq(r->pptr1->const_at(0).copied, 1);
		UT_ASSERTeq(r->pptr1->const_at(1).copied, 1);
		UT_ASSERTeq(r->pptr1->const_at(2).copied, 1);
		UT_ASSERTeq(r->pptr1->const_at(3).copied, 1);

		UT_ASSERTeq(r->pptr1->const_at(0).moved, 0);
		UT_ASSERTeq(r->pptr1->const_at(1).moved, 0);
		UT_ASSERTeq(r->pptr1->const_at(2).moved, 0);
		UT_ASSERTeq(r->pptr1->const_at(3).moved, 0);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/**
	 * Check if copy ctor will construct vector from element's type
	 * copy ctor.
	 */
	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr2 =
				nvobj::make_persistent<vector_type>(*r->pptr1);
		});

		UT_ASSERTeq(r->pptr2->const_at(0).value, 1);
		UT_ASSERTeq(r->pptr2->const_at(1).value, 2);
		UT_ASSERTeq(r->pptr2->const_at(2).value, 3);
		UT_ASSERTeq(r->pptr2->const_at(3).value, 4);

		UT_ASSERTeq(r->pptr2->const_at(0).copied, 2);
		UT_ASSERTeq(r->pptr2->const_at(1).copied, 2);
		UT_ASSERTeq(r->pptr2->const_at(2).copied, 2);
		UT_ASSERTeq(r->pptr2->const_at(3).copied, 2);

		UT_ASSERTeq(r->pptr2->const_at(0).moved, 0);
		UT_ASSERTeq(r->pptr2->const_at(1).moved, 0);
		UT_ASSERTeq(r->pptr2->const_at(2).moved, 0);
		UT_ASSERTeq(r->pptr2->const_at(3).moved, 0);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<vector_type>(r->pptr1);
			nvobj::delete_persistent<vector_type>(r->pptr2);
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
