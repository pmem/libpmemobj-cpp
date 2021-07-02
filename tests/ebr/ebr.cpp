// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "libpmemobj++/detail/ebr.hpp"
#include "thread_helpers.hpp"
#include "unittest.hpp"

/*
 * ebr -- Test Epoch Based Reclamation mechanism.
 */

#define ITERATIONS 500000
#define ELEMENTS 100
#define TOMBSTONE 3

struct data {
	size_t gc_epoch;
	data *ptr;
	std::atomic<bool> is_visible;

	data()
	{
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		VALGRIND_HG_DISABLE_CHECKING(&is_visible, sizeof(is_visible));
#endif
		gc_epoch = TOMBSTONE;
		ptr = nullptr;
		is_visible.store(false);
	}
};

static void
access_obj(data &obj)
{
	if (obj.is_visible.load()) {
		LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(std::memory_order_seq_cst,
						      &obj.is_visible);
		UT_ASSERTne(obj.ptr, nullptr);
	}
}

static void
mock_insert_obj(data &obj)
{
	obj.ptr = &obj;
	LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(std::memory_order_seq_cst,
					       &obj.is_visible);
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
	UT_ASSERT(!obj.is_visible.load());
	obj.ptr = nullptr;
	obj.gc_epoch = TOMBSTONE;
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
						/* object visible */
						mock_remove_obj(
							obj,
							ebr.staging_epoch());
					} else if (obj.gc_epoch == TOMBSTONE) {
						/* object already erased and
						 * destroyed */
						mock_insert_obj(obj);
					} else {
						/* object invisible, but not yet
						 * reclaimed */
						UT_ASSERT(obj.gc_epoch !=
							  TOMBSTONE);
					}

					ebr.sync();

					auto gc_epoch = ebr.gc_epoch();
					for (size_t i = 0; i < ELEMENTS; ++i) {
						if (container[i].gc_epoch ==
						    gc_epoch) {
							mock_destroy_obj(
								container[i]);
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
