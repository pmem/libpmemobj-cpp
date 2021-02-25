// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2021, Intel Corporation */

/*
 * obj_cpp_pool.c -- cpp pool implementation test
 */

#include "unittest.hpp"

#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <string>

namespace nvobj = pmem::obj;

namespace
{

size_t MB = ((size_t)1 << 20);

struct root {
	nvobj::p<int> val;
};

/* emulate no more space in the memory */
void *
null_alloc_func(size_t s)
{
	errno = ENOSPC;
	return nullptr;
}

void
test_pool_exceptions()
{
	/* try catching pool_invalid_argument as a pool_error */
	try {
		throw pmem::pool_invalid_argument("test");
	} catch (pmem::pool_error &e) {
		return;
	}

	UT_ASSERT(0);
}

/*
 * pool_create -- (internal) test pool create
 */
void
pool_create(const char *path, const char *layout, size_t poolsize,
	    unsigned mode)
{
	nvobj::pool<root> pop;
	try {
		pop = nvobj::pool<root>::create(path, layout, poolsize, mode);
		nvobj::persistent_ptr<root> root = pop.root();
		UT_ASSERT(root != nullptr);
	} catch (pmem::pool_invalid_argument &e) {
		UT_OUT("%s: pool::create: %s", path, e.what());
		return;
	} catch (pmem::pool_error &e) {
		UT_OUT("%s: pool::create: (pool_error) %s", path, e.what());
		return;
	}

	os_stat_t stbuf;
	STAT(path, &stbuf);

	UT_OUT("%s: file size %zu mode 0%o", path, stbuf.st_size,
	       stbuf.st_mode & 0777);
	try {
		pop.close();
	} catch (std::logic_error &lr) {
		UT_OUT("%s: pool.close: %s", path, lr.what());
		return;
	}

	int result = nvobj::pool<root>::check(path, layout);

	if (result < 0)
		UT_OUT("!%s: pool::check", path);
	else if (result == 0)
		UT_OUT("%s: pool::check: not consistent", path);
}

/*
 * pool_open -- (internal) test pool open
 */
void
pool_open(const char *path, const char *layout)
{
	nvobj::pool<root> pop;
	try {
		pop = nvobj::pool<root>::open(path, layout);
	} catch (pmem::pool_invalid_argument &e) {
		UT_OUT("%s: pool::open: %s", path, e.what());
		return;
	} catch (pmem::pool_error &e) {
		UT_OUT("%s: pool::open: (pool_error) %s", path, e.what());
		return;
	}

	UT_OUT("%s: pool::open: Success", path);

	try {
		pop.close();
	} catch (std::logic_error &lr) {
		UT_OUT("%s: pool.close: %s", path, lr.what());
	}
}

/*
 * double_close -- (internal) test double pool close
 */
void
double_close(const char *path, const char *layout, size_t poolsize,
	     unsigned mode)
{
	nvobj::pool<root> pop;
	try {
		pop = nvobj::pool<root>::create(path, layout, poolsize, mode);
	} catch (pmem::pool_error &) {
		UT_OUT("!%s: pool::create", path);
		return;
	}

	UT_OUT("%s: pool::create: Success", path);

	try {
		pop.close();
		UT_OUT("%s: pool.close: Success", path);
		pop.close();
	} catch (std::logic_error &lr) {
		UT_OUT("%s: pool.close: %s", path, lr.what());
	}
}

/*
 * get_root_closed -- (internal) test get_root on a closed pool
 */
void
get_root_closed()
{
	nvobj::pool<root> pop;

	try {
		pop.root();
	} catch (pmem::pool_error &pe) {
		UT_OUT("pool.get_root: %s", pe.what());
	}
}

} /* namespace */

static void
test(int argc, char *argv[])
{
	if (argc < 4)
		UT_FATAL("usage: %s op path layout [poolsize mode]", argv[0]);

	const char *layout = nullptr;
	size_t poolsize;
	unsigned mode;

	if (strcmp(argv[3], "EMPTY") == 0)
		layout = "";
	else
		layout = argv[3];

	switch (argv[1][0]) {
		case 'n':
			pmemobj_set_funcs(null_alloc_func, NULL, NULL, NULL);
			/* no break */
		case 'c':
			poolsize = std::stoul(argv[4], nullptr, 0) *
				MB; /* in megabytes */
			mode = static_cast<unsigned>(
				std::stoul(argv[5], nullptr, 8));
			pool_create(argv[2], layout, poolsize, mode);
			break;
		case 'o':
			pool_open(argv[2], layout);
			break;
		case 'd':
			poolsize = std::stoul(argv[4], nullptr, 0) *
				MB; /* in megabytes */
			mode = static_cast<unsigned>(
				std::stoul(argv[5], nullptr, 8));
			double_close(argv[2], layout, poolsize, mode);
			break;
		case 'i':
			get_root_closed();
			break;
		default:
			UT_FATAL("unknown operation");
	}

	test_pool_exceptions();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
