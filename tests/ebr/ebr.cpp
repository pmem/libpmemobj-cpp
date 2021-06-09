// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "libpmemobj++/detail/ebr.hpp"
#include "../common/thread_helpers.hpp"
#include "../common/unittest.hpp"

/*
 * ebr -- Test Epoch Based Reclamation mechanism.
 */

#define ITERATIONS 1000000
#define ELEMENTS 100
#define EPOCHS_NUMBER 3

struct data {
	data *ptr;
	size_t gc_epoch;
	std::atomic<bool> is_visible;

	data()
	{
		ptr = this;
		gc_epoch = EPOCHS_NUMBER;
		is_visible.store(true);
	}
};

static void
access_obj(data &obj)
{
	if (obj.is_visible.load()) {
		if (obj.ptr == nullptr) {
			UT_ASSERT(0);
		}
	}
}

static void
mock_insert_obj(data &obj)
{
	obj.ptr = &obj;
	UT_ASSERT(!obj.is_visible.load());
	obj.is_visible.store(true);
}

static void
mock_remove_obj(data &obj, size_t epoch)
{
	UT_ASSERT(obj.is_visible.load());
	obj.is_visible.store(false);
	obj.gc_epoch = epoch;
}

static void
mock_destroy_obj(data &obj)
{
	obj.ptr = nullptr;
	obj.gc_epoch = EPOCHS_NUMBER;
}

static void
test_ebr()
{
	const size_t threads = 8;
	pmem::detail::ebr ebr;
	data container[ELEMENTS];

	parallel_xexec(
		threads, [&](size_t id, std::function<void(void)> syncthreads) {
			syncthreads();

			size_t iteration = 0;
			auto w = ebr.register_worker();
			while (iteration < ITERATIONS) {
				size_t n = iteration++ % ELEMENTS;

				/* writer */
				if (id == 0) {
					data &obj = container[n];

					if (obj.is_visible.load()) {
						mock_remove_obj(
							obj,
							ebr.staging_epoch());
					} else if (obj.gc_epoch ==
						   EPOCHS_NUMBER) {
						/* object already erased and
						 * destroyed */
						mock_insert_obj(obj);
					}

					ebr.sync();

					auto gc_epoch = ebr.gc_epoch();
					for (size_t i = 0; i < ELEMENTS; ++i) {
						if (obj.gc_epoch == gc_epoch) {
							mock_destroy_obj(obj);
						}
					}
					continue;
				}

				/* reader */
				w.critical([&] { access_obj(container[n]); });
			}
		});
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test_ebr(); });
}
