/*
 * Copyright 2019-2020, Intel Corporation
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

/**
 * @file
 * A persistent version of thread-local storage.
 */

#ifndef LIBPMEMOBJ_CPP_ENUMERABLE_THREAD_SPECIFIC_HPP
#define LIBPMEMOBJ_CPP_ENUMERABLE_THREAD_SPECIFIC_HPP

#include <libpmemobj++/container/segment_vector.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/shared_mutex.hpp>

#include <cassert>
#include <deque>
#include <mutex>
#include <numeric>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

namespace pmem
{
namespace detail
{

/**
 * This structure is used for assigning unique thread ids so that
 * those ids will be reused in case of thread exit.
 *
 * Ids will be between 0 and N where N is max number of threads.
 */
struct id_manager {
	id_manager();

	id_manager(const id_manager &) = delete;
	id_manager &operator=(const id_manager &) = delete;

	size_t get();
	void release(size_t id);

private:
	static constexpr size_t initial_queue_capacity = 1024;

	std::mutex mutex;
	std::size_t queue_capacity;
	std::deque<size_t> queue;
};

/** RAII-style structure for holding thread id */
struct thread_id_type {
	thread_id_type();

	thread_id_type(const thread_id_type &) = delete;
	thread_id_type &operator=(const thread_id_type &) = delete;

	~thread_id_type();

	size_t get();

private:
	size_t id;

	static id_manager &get_id_manager();
};

/**
 * Class for storing thread local data.
 * Needed in concurrent containers for data consistency.
 * Mutex - mutex that satisfies SharedMutex requirements.
 * Storage - persistent container for storing threads data.
 *
 * @pre T - must be default constructable.
 * @pre Mutex must satisfy SharedMutex requirements.
 * @pre Storage must provide methods:
 *    reference emplace_back(Args &&...)
 *    reference operator[](size_type)
 *    size_type size()
 *    bool empty()
 *    void clear()
 * @pre Storage must support iterators
 * @pre Reference to an object in Storage (obtained by operator[])
 *    must be valid until this object is removed.
 */
template <typename T, typename Mutex = obj::shared_mutex,
	  typename Storage =
		  obj::segment_vector<T, obj::exponential_size_array_policy<>>>
class enumerable_thread_specific {
	using storage_type = Storage;
	using mutex_type = Mutex;

public:
	/* traits */
	using value_type = T;
	using size_type = typename storage_type::size_type;
	using difference_type = typename storage_type::difference_type;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using iterator = typename storage_type::iterator;
	using const_iterator = typename storage_type::const_iterator;

	/* initialization */
	template <typename Handler>
	void initialize(Handler handler = [](reference) {});

	/* ctors & dtor */
	enumerable_thread_specific();
	~enumerable_thread_specific() = default;

	/* access */
	reference local();

	/* size */
	bool empty() const;
	void clear();
	size_type size() const;

	/* iterators */
	iterator begin();
	iterator end();
	const_iterator begin() const;
	const_iterator end() const;

private:
	/* private helper methods */
	obj::pool_base get_pool() const noexcept;
	void set_cached_size(size_t s);
	size_t get_cached_size();

	mutex_type _mutex;
	storage_type _storage;

	obj::p<std::atomic<size_t>> _storage_size;
};

inline id_manager::id_manager()
    : queue_capacity(initial_queue_capacity), queue(initial_queue_capacity, 0)
{
	/* Convert 0, 0, 0, ..., 0 into 0, 1, 2, ..., N */
	std::iota(queue.begin(), queue.end(), 0);
}

/**
 * Obtain unique thread id.
 *
 * @pre this method must be called under a lock.
 */
inline size_t
id_manager::get()
{
	std::unique_lock<std::mutex> lock(mutex);

	if (queue.empty())
		queue.push_front(queue_capacity++);

	auto front = queue.front();
	queue.pop_front();

	return front;
}

/**
 * Releases thread id so that it can be reused by other threads.
 *
 * @pre this method must be called under a lock.
 */
inline void
id_manager::release(size_t id)
{
	std::unique_lock<std::mutex> lock(mutex);

	queue.push_front(id);
}

/**
 * Get reference to id_manager instance.
 */
inline id_manager &
thread_id_type::get_id_manager()
{
	static id_manager manager;
	return manager;
}

/**
 * thread_id_type constructor.
 *
 * Obtains id for current thread.
 */
inline thread_id_type::thread_id_type()
{
	auto &manager = get_id_manager();

	/*
	 * Drd had a bug related to static object initialization and reported
	 * conflicting load on std::mutex access inside id_manager.
	 */
#if LIBPMEMOBJ_CPP_VG_DRD_ENABLED
	ANNOTATE_BENIGN_RACE_SIZED(
		&manager, sizeof(std::mutex),
		"https://bugs.kde.org/show_bug.cgi?id=416286");
#endif

	id = manager.get();
}

/**
 * thread_id_type destructor.
 *
 * Releases id associated with current thread.
 */
inline thread_id_type::~thread_id_type()
{
	get_id_manager().release(id);
}

/**
 * Obtain current thread id.
 */
inline size_t
thread_id_type::get()
{
	return id;
}

/**
 * Constructor.
 */
template <typename T, typename Mutex, typename Storage>
enumerable_thread_specific<T, Mutex, Storage>::enumerable_thread_specific()
{
	_storage_size.get_rw() = 0;
}

/**
 * Set cached storage size, persist it and make valgrind annotations.
 */
template <typename T, typename Mutex, typename Storage>
void
enumerable_thread_specific<T, Mutex, Storage>::set_cached_size(size_t s)
{
	auto pop = get_pool();

	/* Helgrind does not understand std::atomic */
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	VALGRIND_HG_DISABLE_CHECKING(&_storage_size, sizeof(_storage_size));
#endif

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED || LIBPMEMOBJ_CPP_VG_DRD_ENABLED
	ANNOTATE_HAPPENS_BEFORE(&_storage_size);
#endif

	_storage_size.get_rw().store(s);
	pop.persist(_storage_size);
}

/**
 * Get cached storage size and make valgrind annotations.
 */
template <typename T, typename Mutex, typename Storage>
size_t
enumerable_thread_specific<T, Mutex, Storage>::get_cached_size()
{
	auto s = _storage_size.get_ro().load();

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED || LIBPMEMOBJ_CPP_VG_DRD_ENABLED
	ANNOTATE_HAPPENS_AFTER(&_storage_size);
#endif

	return s;
}

/**
 * Initialization method. Uses handler functor to each element stored.
 * It must be used to handle the remaining data after a crash and restore
 * the initial state of a container.
 *
 * @post empty() == true.
 */
template <typename T, typename Mutex, typename Storage>
template <typename Handler>
void
enumerable_thread_specific<T, Mutex, Storage>::initialize(Handler handler)
{
	for (reference e : *this) {
		handler(e);
	}
	clear();
}

/**
 * Returns data reference for the current thread.
 * For the new thread, element by reference will be default constructed.
 *
 * @pre must be called outside of a transaction.
 *
 * @return reference to value for the current thread.
 */
template <typename T, typename Mutex, typename Storage>
typename enumerable_thread_specific<T, Mutex, Storage>::reference
enumerable_thread_specific<T, Mutex, Storage>::local()
{
	assert(pmemobj_tx_stage() != TX_STAGE_WORK);

	static thread_local thread_id_type tid;
	auto index = tid.get();

	auto cached_size = get_cached_size();

	if (index >= cached_size) {
		std::unique_lock<mutex_type> lock(_mutex);

		/* Size of the storage could have changed before we obtained the
		 * lock. That's why we read size once again. */
		auto size = _storage.size();

		if (index >= size) {
			_storage.resize(index + 1);
			set_cached_size(index + 1);
		} else if (size != cached_size) {
			set_cached_size(size);
		}
	}

	/*
	 * Because _storage can only grow (unless clear() was called which
	 * should not happen simultaneously with this operation), index must be
	 * less than _storage.size().
	 */
	return _storage[index];
}

/**
 * Removes all elements from the container.
 * Not thread safe.
 *
 * @post empty() == true.
 */
template <typename T, typename Mutex, typename Storage>
void
enumerable_thread_specific<T, Mutex, Storage>::clear()
{
	auto pop = get_pool();

	obj::transaction::run(pop, [&] {
		_storage_size.get_rw() = 0;
		_storage.clear();
	});
}

/**
 * Returns number of elements being stored in the container.
 *
 * @return number of elements in container.
 */
template <typename T, typename Mutex, typename Storage>
typename enumerable_thread_specific<T, Mutex, Storage>::size_type
enumerable_thread_specific<T, Mutex, Storage>::size() const
{
	return _storage.size();
}

/**
 * Determines if container is empty or not.
 *
 * @return true if container is empty, false otherwise.
 */
template <typename T, typename Mutex, typename Storage>
bool
enumerable_thread_specific<T, Mutex, Storage>::empty() const
{
	return _storage.size() == 0;
}

/**
 * Returns an iterator to the beginning.
 *
 * @return iterator to the first element in the container.
 */
template <typename T, typename Mutex, typename Storage>
typename enumerable_thread_specific<T, Mutex, Storage>::iterator
enumerable_thread_specific<T, Mutex, Storage>::begin()
{
	return _storage.begin();
}

/**
 * Returns an iterator to element after the last.
 *
 * @return iterator to the after last element in the container.
 */
template <typename T, typename Mutex, typename Storage>
typename enumerable_thread_specific<T, Mutex, Storage>::iterator
enumerable_thread_specific<T, Mutex, Storage>::end()
{
	return _storage.end();
}

/**
 * Returns an const_iterator to the beginning.
 *
 * @return const_iterator to the first element in the container.
 */
template <typename T, typename Mutex, typename Storage>
typename enumerable_thread_specific<T, Mutex, Storage>::const_iterator
enumerable_thread_specific<T, Mutex, Storage>::begin() const
{
	return _storage.begin();
}

/**
 * Returns an const_iterator to element after the last.
 *
 * @return const_iterator to the after last element in the container.
 */
template <typename T, typename Mutex, typename Storage>
typename enumerable_thread_specific<T, Mutex, Storage>::const_iterator
enumerable_thread_specific<T, Mutex, Storage>::end() const
{
	return _storage.end();
}

/**
 * Private helper function.
 *
 * @pre storage_type must reside in persistent memory pool.
 *
 * @return reference to pool_base object where enumerable_thread_local resides.
 */
template <typename T, typename Mutex, typename Storage>
obj::pool_base
enumerable_thread_specific<T, Mutex, Storage>::get_pool() const noexcept
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);
	return obj::pool_base(pop);
}

} /* namespace detail */
} /* namespace pmem */

#endif
