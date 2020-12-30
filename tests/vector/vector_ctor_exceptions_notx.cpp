// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "../container_generic/ctor_exceptions_notx.hpp"
#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using vector_type = container_t<int>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: vector_ctor_exceptions_notx",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	test_copy_ctor<vector_type>(pop);
	test_default_ctor<vector_type>(pop);
	test_initializer_list_ctor<vector_type>(pop);
	test_iter_iter_ctor<vector_type>(pop);
	test_move_ctor<vector_type>(pop);
	test_size_ctor<vector_type>(pop);
	test_size_value_ctor<vector_type>(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
