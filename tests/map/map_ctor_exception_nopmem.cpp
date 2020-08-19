// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "container_generic/ctor_exceptions_nopmem.hpp"
#include "map_wrapper.hpp"
#include "unittest.hpp"

namespace nvobj = pmem::obj;

using map_type = container_t<int, int>;

struct root {
	nvobj::persistent_ptr<map_type> pptr;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "map_ctor_exceptions_nopmem",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();

	(void)r;
	test_default_ctor<map_type>();
	test_iter_iter_ctor<map_type>();

#if !defined(LIBPMEMOBJ_CPP_TESTS_RADIX)
	test_copy_ctor<map_type>(pop, r->pptr);
	test_initializer_list_ctor<map_type>();
	test_move_ctor<map_type>(pop, r->pptr);
#endif

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
