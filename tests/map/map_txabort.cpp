// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../common/map_wrapper.hpp"
#include "../common/unittest.hpp"
#include "../container_generic/container_txabort.hpp"

#define LAYOUT "map_txabort"

namespace nvobj = pmem::obj;

using map_type = container_t<int, nvobj::p<int>>;

struct root {
	nvobj::persistent_ptr<map_type> pptr1;
	nvobj::persistent_ptr<map_type> pptr2;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	auto r = pop.root();

	test_tx_singlethread<map_type>(pop, r->pptr1, r->pptr2);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
