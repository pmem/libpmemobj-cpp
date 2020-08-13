// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/atomic_self_relative_ptr.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>

template 
using atomic_ptr = std::atomic<pmem::obj::experimental::self_relative_ptr<T>>;

struct root {
	pointer<foo> pfoo;
	pointer<nvobj::p<int>[TEST_ARR_SIZE]> parr;
	pointer_base arr[3];

	/* This variable is unused, but it's here to check if the persistent_ptr
	 * does not violate its own restrictions.
	 */
	pointer<nested<pointer>> outer;
};

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

	pop.close();
}