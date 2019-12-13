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
#include <libpmemobj++/detail/volatile_state.hpp>
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
namespace obj
{
namespace experimental
{

/**
 * This structure is used for assigning unique thread ids so that
 * those ids will be reused in case of thread exit.
 *
 * Ids will be between 0 and N where N is max number of threads.
 */
struct id_manager {
	id_manager();

	size_t get();
	void release(size_t id);

private:
	static constexpr size_t initial_queue_capacity = 1024;

	std::mutex mutex;
	std::size_t queue_capacity;
	std::deque<size_t> queue;
};

/**
 * Class for storing thread local data.
 * Needed in concurrent containers for data consistency.
 * Map - volatile key-value container for storing threads.
 * Mutex - mutex that satisfies SharedMutex requirements.
 * Storage - persistent container for storing threads data.
 *
 * @pre T - must be default constructable.
 * @pre Map must provide methods:
 *    const_iterator find(const key_type&) const
 *    const_iterator cend()
 *    void clear()
 *    insert(const std::pair<const key, value> &val)
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
template <typename T, template <typename...> class Map = std::unordered_map,
	  typename Mutex = pmem::obj::shared_mutex,
	  typename Storage = segment_vector<T, exponential_size_array_policy<>>>
class enumerable_thread_specific {
	/* map traits */
	using storage_type = Storage;
	using map_key_type = size_t;
	using map_value_type = typename storage_type::size_type;
	using map_hash_type = std::hash<map_key_type>;
	using map_type = Map<map_key_type, map_value_type, map_hash_type>;
	/* lock traits */
	using mutex_type = Mutex;
	using scoped_lock_type = std::lock_guard<mutex_type>;

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
	enumerable_thread_specific() = default;
	~enumerable_thread_specific() = default;

	/* access */
	reference local();
	reference local(bool &exists);

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
	map_value_type storage_emplace();
	pool_base get_pool() const noexcept;

	mutex_type _mutex;
	storage_type _storage;

	/** RAII-style structure for holding thread id */
	struct thread_id_type {
		thread_id_type();

		~thread_id_type();

		size_t get();

	private:
		size_t id;
	};

	static id_manager &get_id_manager();
};

id_manager::id_manager()
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
size_t
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
void
id_manager::release(size_t id)
{
	std::unique_lock<std::mutex> lock(mutex);

	queue.push_front(id);
}

/**
 * Get reference to id_manager instance.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
id_manager &
enumerable_thread_specific<T, Map, Mutex, Storage>::get_id_manager()
{
	static id_manager manager;
	return manager;
};

/**
 * thread_id_type constructor.
 *
 * Obtains id for current thread.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
enumerable_thread_specific<T, Map, Mutex,
			   Storage>::thread_id_type::thread_id_type()
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
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
enumerable_thread_specific<T, Map, Mutex,
			   Storage>::thread_id_type::~thread_id_type()
{
	get_id_manager().release(id);
}

/**
 * Obtain current thread id.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
size_t
enumerable_thread_specific<T, Map, Mutex, Storage>::thread_id_type ::get()
{
	return id;
}

/**
 * Initialization method. Uses handler functor to each element stored.
 * It must be used to handle the remaining data after a crash and restore
 * the initial state of a container.
 *
 * @post empty() == true.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
template <typename Handler>
void
enumerable_thread_specific<T, Map, Mutex, Storage>::initialize(Handler handler)
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
 * @return reference to value for the current thread.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::reference
enumerable_thread_specific<T, Map, Mutex, Storage>::local()
{
	bool exists;
	return local(exists);
}

/**
 * Returns data reference for the current thread.
 * For the new thread, element by reference will be default constructed.
 *
 * @pre must be called outside of a transaction.
 *
 * @return reference to value for the current thread.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::reference
enumerable_thread_specific<T, Map, Mutex, Storage>::local(bool &exists)
{
	assert(pmemobj_tx_stage() != TX_STAGE_WORK);

	static thread_local thread_id_type tid;

	const map_key_type key = tid.get();
	auto map = detail::volatile_state::get<map_type>(pmemobj_oid(this));

	{
		std::shared_lock<mutex_type> lock(_mutex);

		typename map_type::const_iterator it = map->find(key);
		exists = it != map->end();
		/* return value if thread already exists in map */
		if (exists)
			return _storage[it->second];
	}

	/* always releasing a mutex, but no need to search through map because
	 * no other thread has the same id and cannot insert such key */

	{
		std::unique_lock<mutex_type> lock(_mutex);
		/* checking if thread id is not presented in map */
		assert(map->find(key) == map->end());

		/* create bucket if it's not found */
		auto value = storage_emplace();
		map->insert(typename map_type::value_type(key, value));
		return _storage[value];
	}
}

/**
 * Private helper function. Default constructs element.
 * Should be called only when _mutex is taken in exclusive mode.
 *
 * @return value_type of map_type, that indexing the added item of storage.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::map_value_type
enumerable_thread_specific<T, Map, Mutex, Storage>::storage_emplace()
{
	_storage.emplace_back();
	return _storage.size() - 1;
}

/**
 * Removes all elements from the container.
 * Not thread safe.
 *
 * @post empty() == true.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
void
enumerable_thread_specific<T, Map, Mutex, Storage>::clear()
{
	detail::volatile_state::destroy(pmemobj_oid(this));
	_storage.clear();
}

/**
 * Returns number of elements being stored in the container.
 *
 * @return number of elements in container.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::size_type
enumerable_thread_specific<T, Map, Mutex, Storage>::size() const
{
	return _storage.size();
}

/**
 * Determines if container is empty or not.
 *
 * @return true if container is empty, false overwise.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
bool
enumerable_thread_specific<T, Map, Mutex, Storage>::empty() const
{
	return _storage.size() == 0;
}

/**
 * Returns an iterator to the beginning.
 *
 * @return iterator to the first element in the container.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::iterator
enumerable_thread_specific<T, Map, Mutex, Storage>::begin()
{
	return _storage.begin();
}

/**
 * Returns an iterator to element after the last.
 *
 * @return iterator to the after last element in the container.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::iterator
enumerable_thread_specific<T, Map, Mutex, Storage>::end()
{
	return _storage.end();
}

/**
 * Returns an const_iterator to the beginning.
 *
 * @return const_iterator to the first element in the container.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::const_iterator
enumerable_thread_specific<T, Map, Mutex, Storage>::begin() const
{
	return _storage.begin();
}

/**
 * Returns an const_iterator to element after the last.
 *
 * @return const_iterator to the after last element in the container.
 */
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::const_iterator
enumerable_thread_specific<T, Map, Mutex, Storage>::end() const
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
template <typename T, template <typename...> class Map, typename Mutex,
	  typename Storage>
pool_base
enumerable_thread_specific<T, Map, Mutex, Storage>::get_pool() const noexcept
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);
	return pool_base(pop);
}

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */

#endif
