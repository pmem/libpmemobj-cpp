// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <atomic>

static size_t const concurrency = 4;

int
main()
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
		       });
}
