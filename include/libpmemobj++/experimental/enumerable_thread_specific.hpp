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

/**
 * A persistent version of thread-local storage.
 */

#ifndef LIBPMEMOBJ_CPP_ENUMERABLE_THREAD_SPECIFIC_HPP
#define LIBPMEMOBJ_CPP_ENUMERABLE_THREAD_SPECIFIC_HPP

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/experimental/segment_vector.hpp>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/shared_mutex.hpp>

#include <mutex>
#include <thread>
#include <unordered_map>

namespace pmem
{
namespace obj
{
namespace experimental
{

namespace enumerable_thread_specific_internal
{

/**
 * Simple scoped lock for SharedMutex compatible mutexes.
 * Made to simplify enumerable_thread_specific's code.
 */
template <typename T>
class scoped_lock {
public:
	using rw_mutex_type = T;

	scoped_lock(rw_mutex_type &m, bool write = true)
	{
		mutex = &m;
		is_writer = write;

		if (write) {
			m.lock();
		} else {
			m.lock_shared();
		}
	}

	/* no need to return bool, because we always releasing the mutex */
	void
	upgrade_to_writer()
	{
		assert(mutex);
		if (is_writer)
			return;
		mutex->unlock_shared();
		mutex->lock();
		is_writer = true;
	}

	~scoped_lock()
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

private:
	rw_mutex_type *mutex;
	bool is_writer;
};

} /* enumerable_thread_specific_internal */

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
 *    XXX: used instead of emplace (libpmemobj-cpp tbb version update required)
 *    reference operator[](key_type)
 * @pre Mutex must satisfy SharedMutex requirements.
 * @pre Storage must provide methods:
 *    reference emplace_back(Args &&...)
 *    reference operator[](size_type)
 *    size_type size()
 *    bool empty()
 *    void clear()
 * @pre Storage must support iterators.
 */
template <typename T, template <typename...> typename Map = std::unordered_map,
	  typename Mutex = pmem::obj::shared_mutex,
	  typename Storage = segment_vector<T>>
class enumerable_thread_specific {
	/* map traits */
	using storage_type = Storage;
	using map_key_type = typename std::thread::id;
	using map_value_type = typename storage_type::size_type;
	using map_hash_type = std::hash<map_key_type>;
	using map_type = Map<map_key_type, map_value_type, map_hash_type>;
	/* lock traits */
	using map_rw_mutex = Mutex;
	using map_scoped_lock =
		enumerable_thread_specific_internal::scoped_lock<map_rw_mutex>;
	using storage_mutex = pmem::obj::mutex;
	using storage_scoped_lock = std::lock_guard<storage_mutex>;

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
	using reverse_iterator = typename storage_type::reverse_iterator;
	using const_reverse_iterator =
		typename storage_type::const_reverse_iterator;

	/* initialization */
	template <typename Handler>
	void initialize(Handler handler = [](reference) {});

	/* ctors & dtor */
	enumerable_thread_specific();
	enumerable_thread_specific(enumerable_thread_specific &other);
	enumerable_thread_specific(enumerable_thread_specific &&other);
	~enumerable_thread_specific();

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
	/**
	 * Private helper wrapper class for map_type.
	 * Needed to construct map_type through v<>.get().
	 *
	 * We can't use v<map_type *> because in case of crash and reboot:
	 * enumerable_thread_specific's constructor will not be called,
	 * v<>.get() will construct pointer, not the map,
	 * and thread mapping will not restore.
	 */
	class map_wrapper {
	public:
		map_wrapper()
		{
			p = new map_type();
		}
		map_wrapper(const map_wrapper &rhs)
		{
			p = new map_type(*rhs.p);
		}
		~map_wrapper()
		{
			delete p;
		}
		map_type *operator->()
		{
			return p;
		}

	private:
		/* Storing a pointer is necessary for a consistent layout */
		map_type *p;
	};

private:
	/* private helper methods */
	map_value_type storage_emplace();
	pool_base get_pool() const noexcept;

	v<map_wrapper> _map;
	map_rw_mutex _map_mutex;

	storage_type _storage;
	storage_mutex _storage_mutex;
};

/**
 * Initialization method. Uses handler functor to each element stored.
 * It must be used to handle the remaining data after a crash and restore
 * the initial state of a container.
 *
 * @post empty() == true.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
template <typename Handler>
void
enumerable_thread_specific<T, Map, Mutex, Storage>::initialize(Handler handler)
{
	for (reference e : *this) {
		handler(e);
	}
	_storage.clear();
}

/**
 * Dafault constructor.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
enumerable_thread_specific<T, Map, Mutex, Storage>::enumerable_thread_specific()
{
}

/**
 * Copy constructor.
 *
 * @param[in] other reference to the enumerable_thread_specific to copy.
 *
 * @post size() == other.size().
 */
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
enumerable_thread_specific<T, Map, Mutex, Storage>::enumerable_thread_specific(
	enumerable_thread_specific &other)
{
	_map.get(other._map.get());
	_storage = other._storage;
}

/**
 * Move constructor.
 *
 * @param[in] other rvalue reference to the enumerable_thread_specific to move.
 *
 * @post size() == other.size().
 * @post other.empty() == true.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
enumerable_thread_specific<T, Map, Mutex, Storage>::enumerable_thread_specific(
	enumerable_thread_specific &&other)
{
	_map.get(std::move(other._map.get()));
	_storage = std::move(other._storage);
}

/**
 * Dafault destructor.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
enumerable_thread_specific<T, Map, Mutex,
			   Storage>::~enumerable_thread_specific()
{
	/* XXX: will not be called in case of transaction abort */
	/* must be called manually */
	_map.get().~map_wrapper();
}

/**
 * Returns data reference for the current thread.
 * For the new thread, element by reference will be default constructed.
 *
 * @return reference to value for the current thread.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
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
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::reference
enumerable_thread_specific<T, Map, Mutex, Storage>::local(bool &exists)
{
	assert(pmemobj_tx_stage() != TX_STAGE_WORK);

	const map_key_type key = std::this_thread::get_id();
	map_wrapper &map = _map.get();

	/* read lock to try find key in map */
	map_scoped_lock lock(_map_mutex, false);

	typename map_type::const_iterator it = map->find(key);
	exists = it != map->cend();
	/* return value if thread already exists in map */
	if (exists) {
		return _storage[(*it).second];
	}

	lock.upgrade_to_writer();

	/* checking if thread id is not presented in map */
	assert(map->find(key) == map->cend());

	/* create bucket if it's not found */
	map_value_type value = storage_emplace();
	/* XXX: should be emplace, see class description */
	map->operator[](key) = value;
	return _storage[value];
}

/**
 * Private helper function. Default constructs element. Captures mutex.
 *
 * @return value_type of map_type, that indexing the added item of storage.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
typename enumerable_thread_specific<T, Map, Mutex, Storage>::map_value_type
enumerable_thread_specific<T, Map, Mutex, Storage>::storage_emplace()
{
	storage_scoped_lock lock(_storage_mutex);

	_storage.emplace_back();
	return _storage.size() - 1;
}

/**
 * Removes all elements from the container.
 * Not thread safe.
 *
 * @post empty() == true.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
void
enumerable_thread_specific<T, Map, Mutex, Storage>::clear()
{
	_map.get()->clear();
	_storage.clear();
}

/**
 * Returns number of elements being stored in the container.
 *
 * @return number of elements in container.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
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
template <typename T, template <typename...> typename Map, typename Mutex,
	  typename Storage>
bool
enumerable_thread_specific<T, Map, Mutex, Storage>::empty() const
{
	return _storage.empty();
}

/**
 * Returns an iterator to the beginning.
 *
 * @return iterator to the first element in the container.
 */
template <typename T, template <typename...> typename Map, typename Mutex,
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
template <typename T, template <typename...> typename Map, typename Mutex,
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
template <typename T, template <typename...> typename Map, typename Mutex,
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
template <typename T, template <typename...> typename Map, typename Mutex,
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
template <typename T, template <typename...> typename Map, typename Mutex,
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
