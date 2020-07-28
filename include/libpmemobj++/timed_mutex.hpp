// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/**
 * @file
 * Pmem-resident timed_mutex.
 */

#ifndef LIBPMEMOBJ_CPP_TIMED_MUTEX_HPP
#define LIBPMEMOBJ_CPP_TIMED_MUTEX_HPP

#include <chrono>

#include <libpmemobj++/detail/conversions.hpp>
#include <libpmemobj/thread.h>

namespace pmem
{

namespace obj
{

/**
 * Persistent memory resident timed_mutex implementation.
 *
 * This class is an implementation of a PMEM-resident timed_mutex
 * which mimics in behavior the C++11 std::timed_mutex. This class
 * satisfies all requirements of the TimedMutex and StandardLayoutType
 * concepts. The typical usage example would be:
 * @snippet mutex/mutex.cpp timed_mutex_example
 */
class timed_mutex {
	typedef std::chrono::system_clock clock_type;

public:
	/** Implementation defined handle to the native type. */
	typedef PMEMmutex *native_handle_type;

	/**
	 * Default constructor.
	 *
	 * @throw lock_error when the timed_mutex is not from persistent memory.
	 */
	timed_mutex()
	{
		PMEMobjpool *pop;
		if ((pop = pmemobj_pool_by_ptr(&plock)) == nullptr)
			throw pmem::lock_error(
				1, std::generic_category(),
				"Persistent mutex not from persistent memory.");

		pmemobj_mutex_zero(pop, &plock);
	}

	/**
	 * Defaulted destructor.
	 */
	~timed_mutex() = default;

	/**
	 * Locks the mutex, blocks if already locked.
	 *
	 * If a different thread already locked this mutex, the calling
	 * thread will block. If the same thread tries to lock a mutex
	 * it already owns, the behavior is undefined.
	 *
	 * @throw lock_error when an error occurs, this includes all
	 * system related errors with the underlying implementation of
	 * the mutex.
	 */
	void
	lock()
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		if (int ret = pmemobj_mutex_lock(pop, &this->plock))
			throw pmem::lock_error(ret, std::system_category(),
					       "Failed to lock a mutex.")
				.with_pmemobj_errormsg();
	}

	/**
	 * Tries to lock the mutex, returns regardless if the lock
	 * succeeds.
	 *
	 * If the same thread tries to lock a mutex it already owns,
	 * the behavior is undefined.
	 *
	 * @return `true` on successful lock acquisition, `false`
	 * otherwise.
	 *
	 * @throw lock_error when an error occurs, this includes all
	 * system related errors with the underlying implementation of
	 * the mutex.
	 */
	bool
	try_lock()
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		int ret = pmemobj_mutex_trylock(pop, &this->plock);

		if (ret == 0)
			return true;
		else if (ret == EBUSY)
			return false;
		else
			throw pmem::lock_error(ret, std::system_category(),
					       "Failed to lock a mutex.")
				.with_pmemobj_errormsg();
	}

	/**
	 * Makes the current thread block until the lock is acquired or a
	 * specific time is reached.
	 *
	 * If the same thread tries to lock a mutex it already owns,
	 * the behavior is undefined.
	 *
	 * @param[in] timeout_time a specific point in time, which when
	 * reached unblocks the thread.
	 *
	 * @return `true` on successful lock acquisition, `false`
	 * otherwise.
	 *
	 * @throw lock_error when an error occurs, this includes all
	 * system related errors with the underlying implementation of
	 * the mutex.
	 */
	template <typename Clock, typename Duration>
	bool
	try_lock_until(
		const std::chrono::time_point<Clock, Duration> &timeout_time)
	{
		return timedlock_impl(timeout_time);
	}

	/**
	 * Makes the current thread block until the lock is acquired or a
	 * specified amount of time passes.
	 *
	 * If the same thread tries to lock a mutex it already owns,
	 * the behavior is undefined.
	 *
	 * @param[in] timeout_duration a specific duration, which when
	 * expired unblocks the thread.
	 *
	 * @return `true` on successful lock acquisition, `false`
	 * otherwise.
	 *
	 * @throw lock_error when an error occurs, this includes all
	 * system related errors with the underlying implementation of
	 * the mutex.
	 */
	template <typename Rep, typename Period>
	bool
	try_lock_for(const std::chrono::duration<Rep, Period> &timeout_duration)
	{
		return timedlock_impl(clock_type::now() + timeout_duration);
	}

	/**
	 * Unlocks a previously locked mutex.
	 *
	 * Unlocking a mutex that has not been locked by the current
	 * thread results in undefined behavior. Unlocking a mutex that
	 * has not been lock also results in undefined behavior.
	 */
	void
	unlock()
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		int ret = pmemobj_mutex_unlock(pop, &this->plock);
		if (ret)
			throw pmem::lock_error(ret, std::system_category(),
					       "Failed to unlock a mutex.")
				.with_pmemobj_errormsg();
	}

	/**
	 * Access a native handle to this condition variable.
	 *
	 * @return a pointer to PMEMmutex.
	 */
	native_handle_type
	native_handle() noexcept
	{
		return &this->plock;
	}

	/**
	 * Deleted assignment operator.
	 */
	timed_mutex &operator=(const timed_mutex &) = delete;

	/**
	 * Deleted copy constructor.
	 */
	timed_mutex(const timed_mutex &) = delete;

private:
	/**
	 * Internal implementation of the timed lock call.
	 */
	template <typename Clock, typename Duration>
	bool
	timedlock_impl(const std::chrono::time_point<Clock, Duration> &abs_time)
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);

		/* convert to my clock */
		const typename Clock::time_point their_now = Clock::now();
		const clock_type::time_point my_now = clock_type::now();
		const auto delta = abs_time - their_now;
		const auto my_abs = my_now + delta;

		struct timespec ts = detail::timepoint_to_timespec(my_abs);

		auto ret = pmemobj_mutex_timedlock(pop, &this->plock, &ts);

		if (ret == 0)
			return true;
		else if (ret == ETIMEDOUT)
			return false;
		else
			throw pmem::lock_error(ret, std::system_category(),
					       "Failed to lock a mutex");
	}

	/** A POSIX style PMEM-resident timed_mutex.*/
	PMEMmutex plock;
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_TIMED_MUTEX_HPP */
