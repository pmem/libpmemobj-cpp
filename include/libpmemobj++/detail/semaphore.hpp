// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#if __cpp_lib_semaphore

#include <limits>
#include <semaphore>

#else

#include <condition_variable>
#include <mutex>

#endif /*  __cpp_lib_semaphore */

namespace pmem
{

namespace detail
{

constexpr ptrdiff_t least_max_value = std::numeric_limits<ptrdiff_t>::max();
#if __cpp_lib_semaphore

template <std::ptrdiff_t LeastMaxValue = least_max_value>
using counting_semaphore = std::counting_semaphore<LeastMaxValue>;
using binary_semaphore = std::binary_semaphore;
#else

/**
 * Our partial std::counting_semaphore implementation.
 *
 * If C++20's std::counting_semaphore implementation is not available, this one
 * is used for threads synchronization
 */
template <std::ptrdiff_t LeastMaxValue = least_max_value>
class counting_semaphore {
public:
	counting_semaphore(ptrdiff_t count = 0) : count_(count)
	{
	}
	~counting_semaphore() = default;

	counting_semaphore(const counting_semaphore &) = delete;
	counting_semaphore &operator=(const counting_semaphore &) = delete;

	/**
	 * Increments the internal counter by the value of update.
	 * Threads waiting in acquire for the counter greater than 0 will be
	 * unblocked.
	 */
	void
	release(std::ptrdiff_t update = 1)
	{
		count_ += update;
		cv.notify_one();
	}

	/**
	 * Blocks until internal couner is greater than 0 and decrements it.
	 */
	void
	acquire()
	{
		std::unique_lock<std::mutex> lock(mtx);
		cv.wait(lock, [&] { return count_ > 0; });
		count_--;
	}

private:
	std::mutex mtx;
	std::condition_variable cv;
	ptrdiff_t count_;
};

using binary_semaphore = counting_semaphore<1>;

#endif /*  __cpp_lib_semaphore */

} /* namespace detail */
} /* namespace pmem */
