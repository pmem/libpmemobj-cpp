// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "ptr_arith.hpp"

#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

#define LAYOUT "cpp"

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool_base pop;

	try {
		pop = nvobj::pool_base::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
					       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_arith<nvobjexp::self_relative_ptr>(pop);
	test_relational<nvobjexp::self_relative_ptr>(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
