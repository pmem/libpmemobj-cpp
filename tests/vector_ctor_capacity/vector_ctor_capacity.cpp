// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>

namespace nvobj = pmem::obj;

const static size_t pool_size = PMEMOBJ_MIN_POOL * 2;
const static size_t test_val1 = 123U;

using vector_type = container_t<int>;

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
	auto pop = nvobj::pool<root>::create(path,
					     "VectorTest: vector_ctor_capacity",
					     pool_size, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr1 = nvobj::make_persistent<vector_type>();
			/* test capacity of default-constructed vector */
			UT_ASSERT(0 == r->pptr1->capacity());
			nvobj::delete_persistent<vector_type>(r->pptr1);

			r->pptr1 = nvobj::make_persistent<vector_type>(
				test_val1, 0);
			/* test capacity of size-value-constructed vector */
			UT_ASSERT(expected_capacity(test_val1) ==
				  r->pptr1->capacity());

			r->pptr2 = nvobj::make_persistent<vector_type>(
				r->pptr1->begin(), r->pptr1->end());
			/* test capacity of iter-iter-constructed vector */
			UT_ASSERT(expected_capacity(test_val1) ==
				  r->pptr2->capacity());
			nvobj::delete_persistent<vector_type>(r->pptr2);

			r->pptr2 =
				nvobj::make_persistent<vector_type>(*r->pptr1);
			/* test capacity of copy-constructed vector */
			UT_ASSERT(expected_capacity(test_val1) ==
				  r->pptr2->capacity());
			nvobj::delete_persistent<vector_type>(r->pptr2);

			r->pptr2 = nvobj::make_persistent<vector_type>(
				std::move(*r->pptr1));
			/* test capacity of move-constructed vector */
			UT_ASSERT(expected_capacity(test_val1) ==
				  r->pptr2->capacity());
			nvobj::delete_persistent<vector_type>(r->pptr2);
			nvobj::delete_persistent<vector_type>(r->pptr1);
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
