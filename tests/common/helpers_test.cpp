// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * helpers_test - test for all helper classes/functions used in testing
 * framework
 */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <atomic>

static size_t const concurrency = 4;

/* Verify if syncthreads is working correctly */
static void
test()
{
	std::atomic<int> counter;
	counter = 0;

	parallel_xexec(concurrency,
		       [&](size_t id, std::function<void()> syncthreads) {
			       counter++;

			       syncthreads();
			       UT_ASSERTeq(counter.load(), concurrency);
			       syncthreads();

			       counter++;

			       syncthreads();
			       UT_ASSERTeq(counter.load(), concurrency * 2);
			       syncthreads();
                   UT_ASSERTeq(counter.load(), concurrency * 2);
		       });
}

int
main()
{
	return run_test(test);
}
