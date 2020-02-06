// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include "unittest.hpp"

struct root {
};

using Object = int[10240];

void
run_ctl_pool_prefault(pmem::obj::pool<struct root> &pop)
try {
	pop.ctl_set<int>("prefault.at_open", 1);

	auto prefault_at_open = pop.ctl_get<int>("prefault.at_open");
	UT_ASSERTeq(prefault_at_open, 1);

	pop.ctl_set<int>("prefault.at_open", 0);

	prefault_at_open = pop.ctl_get<int>("prefault.at_open");
	UT_ASSERTeq(prefault_at_open, 0);
} catch (...) {
	UT_ASSERT(0);
}

void
run_ctl_pool_extend(pmem::obj::pool<struct root> &pop)
try {
	/* disable auto-extend */
	pop.ctl_set<uint64_t>("heap.size.granularity", 0);

	pmem::obj::persistent_ptr<Object> ptr;
	try {
		/* allocate until OOM */
		while (true) {
			pmem::obj::make_persistent_atomic<Object>(pop, ptr);
		}
	} catch (...) {
	}

	pop.ctl_exec<uint64_t>("heap.size.extend", (1 << 20) * 10);

	/* next allocation should succeed */
	try {
		pmem::obj::make_persistent_atomic<Object>(pop, ptr);
	} catch (...) {
		UT_ASSERT(0);
	}
} catch (...) {
	UT_ASSERT(0);
}

void
run_ctl_global()
try {
	pmem::obj::ctl_set<int>("prefault.at_create", 1);

	auto prefault_at_create = pmem::obj::ctl_get<int>("prefault.at_create");
	UT_ASSERTeq(prefault_at_create, 1);

	pmem::obj::ctl_set<int>("prefault.at_create", 0);

	prefault_at_create = pmem::obj::ctl_get<int>("prefault.at_create");
	UT_ASSERTeq(prefault_at_create, 0);
} catch (...) {
	UT_ASSERT(0);
}

void
run_ctl_exception()
{
	try {
		/* run query with non-existing entry point */
		pmem::obj::ctl_set<int>("prefault.non_existing_entry_point", 1);
		UT_ASSERT(0);
	} catch (pmem::ctl_error &e) {
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		/* run query with non-existing entry point */
		pmem::obj::ctl_get<int>("prefault.non_existing_entry_point");
		UT_ASSERT(0);
	} catch (pmem::ctl_error &e) {
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		/* run query with non-existing entry point */
		pmem::obj::ctl_exec<int>("prefault.non_existing_entry_point",
					 1);
		UT_ASSERT(0);
	} catch (pmem::ctl_error &e) {
	} catch (...) {
		UT_ASSERT(0);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = pmem::obj::pool<root>::create(path, "ctl_test", 0,
						 S_IWUSR | S_IRUSR);

	run_ctl_pool_prefault(pop);
	run_ctl_pool_extend(pop);
	run_ctl_global();
	run_ctl_exception();

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
