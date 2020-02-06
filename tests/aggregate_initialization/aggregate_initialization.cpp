// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <iostream>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

struct foo {
	nvobj::p<int> a;
	nvobj::p<int> b;
};

struct root {
	nvobj::persistent_ptr<foo> pfoo;
};

void
test_aggregate(nvobj::pool<root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

#if !__cpp_lib_is_aggregate
	/* make sure aggregate initialization is available */
	UT_ASSERT(0);
#endif

	try {
		nvobj::transaction::run(pop, [&] {
			r->pfoo = nvobj::make_persistent<foo>(2, 3);

			UT_ASSERTeq(r->pfoo->a, 2);
			UT_ASSERTeq(r->pfoo->b, 3);

			nvobj::delete_persistent<foo>(r->pfoo);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_aggregate(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
