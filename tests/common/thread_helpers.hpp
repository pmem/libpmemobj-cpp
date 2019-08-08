// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */
#ifndef THREAD_HELPERS_COMMON_HPP
#define THREAD_HELPERS_COMMON_HPP

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

template <typename Function>
void
parallel_exec(size_t concurrency, Function f)
{
	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back(f, i);
	}

	for (auto &t : threads) {
		t.join();
	}
}

/*
 * This function executes 'concurrency' threads and provides
 * 'syncthreads' method (synchronization barrier) for f()
 */
template <typename Function>
void
parallel_xexec(size_t concurrency, Function f)
{
	std::condition_variable cv;
	std::mutex m;
	size_t counter = 0;

	auto syncthreads = [&] {
		std::unique_lock<std::mutex> lock(m);
		counter++;
		if (counter < concurrency)
			cv.wait(lock);
		else
			/*
			 * notify_call could be called outside of a lock
			 * (it would perform better) but drd complains
			 * in that case
			 */
			cv.notify_all();
	};

	parallel_exec(concurrency, [&](size_t tid) { f(tid, syncthreads); });
}

/*
 * This function executes 'concurrency' threads and wait for all of them to
 * finish executing f before calling join().
 */
template <typename Function>
void
parallel_exec_with_sync(size_t concurrency, Function f)
{
	parallel_xexec(concurrency,
		       [&](size_t tid, std::function<void(void)> syncthreads) {
			       f(tid);

			       syncthreads();
		       });
}

#endif /* THREAD_HELPERS_COMMON_HPP */
