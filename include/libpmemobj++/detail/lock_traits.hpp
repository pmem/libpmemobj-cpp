/*
 * Copyright 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <libpmemobj++/detail/template_helpers.hpp>

#include <cassert>

namespace pmem
{

namespace detail
{

template <typename SharedMutexT>
class shared_mutex_scoped_lock {
	using rw_mutex_type = SharedMutexT;

public:
	shared_mutex_scoped_lock(const shared_mutex_scoped_lock &) = delete;
	shared_mutex_scoped_lock &
	operator=(const shared_mutex_scoped_lock &) = delete;

	/** Default constructor. Construct lock that has not acquired a mutex.*/
	shared_mutex_scoped_lock() : mutex(nullptr), is_writer(false)
	{
	}

	/** Acquire lock on given mutex. */
	shared_mutex_scoped_lock(rw_mutex_type &m, bool write = true)
	    : mutex(nullptr)
	{
		acquire(m, write);
	}

	/** Release lock (if lock is held). */
	~shared_mutex_scoped_lock()
	{
		if (mutex)
			release();
	}

	/** Acquire lock on given mutex. */
	void
	acquire(rw_mutex_type &m, bool write = true)
	{
		is_writer = write;
		mutex = &m;
		if (write)
			mutex->lock();
		else
			mutex->lock_shared();
	}

	/**
	 * Release lock.
	 */
	void
	release()
	{
		assert(mutex);
		rw_mutex_type *m = mutex;
		mutex = nullptr;
		if (is_writer) {
			m->unlock();
		} else {
			m->unlock_shared();
		}
	}

	/**
	 * Try acquire lock on given mutex.
	 *
	 */
	bool
	try_acquire(rw_mutex_type &m, bool write = true)
	{
		assert(!mutex);
		bool result;
		is_writer = write;
		result = write ? m.try_lock() : m.try_lock_shared();
		if (result)
			mutex = &m;
		return result;
	}

	bool
	writer() const
	{
		return shared_mutex_scoped_lock::is_writer;
	}

	rw_mutex_type *
	get()
	{
		return mutex;
	}

protected:
	/**
	 * The pointer to the current mutex that is held, or NULL if no mutex is
	 * held.
	 */
	rw_mutex_type *mutex;
	/**
	 * If mutex!=NULL, then is_writer is true if holding a writer lock,
	 * false if holding a reader lock. Not defined if not holding a lock.
	 */
	bool is_writer;
}; /* class shared_mutex_scoped_lock */

template <typename ScopedLockType>
using scoped_lock_upgrade_to_writer =
	decltype(std::declval<ScopedLockType>().upgrade_to_writer());

template <typename ScopedLockType>
using scoped_lock_has_upgrade_to_writer =
	detail::supports<ScopedLockType, scoped_lock_upgrade_to_writer>;

template <typename ScopedLockType>
using scoped_lock_downgrade_to_reader =
	decltype(std::declval<ScopedLockType>().downgrade_to_reader());

template <typename ScopedLockType>
using scoped_lock_has_downgrade_to_reader =
	detail::supports<ScopedLockType, scoped_lock_downgrade_to_reader>;

template <typename ScopedLockType,
	  bool = scoped_lock_has_upgrade_to_writer<ScopedLockType>::value
		  &&scoped_lock_has_downgrade_to_reader<ScopedLockType>::value>
class scoped_lock_traits {
public:
	using scope_lock_type = ScopedLockType;

	static bool
	initial_rw_state(bool write)
	{
		/* For upgradeable locks, initial state is always read */
		return false;
	}

	static bool
	upgrade_to_writer(scope_lock_type &lock)
	{
		return lock.upgrade_to_writer();
	}

	static bool
	downgrade_to_reader(scope_lock_type &lock)
	{
		return lock.downgrade_to_reader();
	}
};

template <typename ScopedLockType>
class scoped_lock_traits<ScopedLockType, false> {
public:
	using scope_lock_type = ScopedLockType;

	static bool
	initial_rw_state(bool write)
	{
		/* For non-upgradeable locks, we take lock in required mode
		 * immediately */
		return write;
	}

	static bool
	upgrade_to_writer(scope_lock_type &lock)
	{
		if (!lock.writer()) {
			auto m = lock.get();
			lock.release();
			lock.acquire(*m, true);
			return false;
		}
		/* This overload is for locks which do not support upgrade
		 * operation. For those locks, upgrade_to_writer should not be
		 * called when holding a read lock */
		return true;
	}

	static bool
	downgrade_to_reader(scope_lock_type &lock)
	{
		/* This overload is for locks which do not support downgrade
		 * operation. For those locks, downgrade_to_reader should never
		 * be called */
		assert(false);

		return false;
	}
};

} /* namespace detail */
} /* namespace pmem */
