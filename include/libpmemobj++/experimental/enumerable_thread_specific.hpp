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
 * @file
 * A persistent version of thread-local storage.
 */

#ifndef PMEMOBJ_ENUMERABLE_THREAD_SPECIFIC_HPP
#define PMEMOBJ_ENUMERABLE_THREAD_SPECIFIC_HPP

#include <libpmemobj++/detail/lock_traits.hpp>
#include <libpmemobj++/experimental/segment_vector.hpp>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/shared_mutex.hpp>

#include <thread>
#include <unordered_map>

namespace pmem
{
namespace obj
{
namespace experimental
{

/**
 * Default MapTraits for enumerable_thread_specific.
 */
class default_map_traits {
public:
	using map_type = std::unordered_map<std::thread::id, size_t>;
	using map_rw_mutex = pmem::obj::shared_mutex;
	using map_scoped_lock =
		pmem::detail::shared_mutex_scoped_lock<map_rw_mutex>;
};

/**
 * Class for storing thread local data.
 * Needed in concurrent containers for data consistency.
 * MapTraits - class that determines volatile key-value container(for storing
 * threads), read-write mutex and scoped lock.
 * Storage - persistent container for storing threads data.
 *
 * @pre T - must be default constructable.
 * @pre MapTraits must define map_type(key - std::thread::id, value - size_t),
 * map_rw_mutex, map_scoped_lock.
 * @pre MapTraits must support iterators.
 * @pre MapTraits::map_type must provide methods:
 *    const_iterator find(const key_type&) const
 *    void clear()
 * @pre Storage must provide methods:
 *    reference emplace_back(Args &&...)
 *    reference operator[](size_type)
 *    size_type size()
 *    bool empty()
 *    void clear()
 * @pre Storage must support iterators.
 */
template <typename T, typename MapTraits = default_map_traits,
	  typename Storage = segment_vector<T>>
class enumerable_thread_specific {
	/* map traits */
	using map_traits = MapTraits;
	using storage_type = Storage;
	using map_key_type = typename std::thread::id;
	using map_value_type = typename storage_type::size_type;
	using map_type = typename map_traits::map_type;
	/* lock traits */
	using map_rw_mutex = typename map_traits::map_rw_mutex;
	using map_scoped_lock = typename map_traits::map_scoped_lock;
	using lock_traits = pmem::detail::scoped_lock_traits<map_scoped_lock>;
	using storage_mutex = pmem::obj::mutex;

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

	/* ctors & dtor */
	enumerable_thread_specific();
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
	/* private helper methods */
	map_value_type storage_emplace();
	pool_base get_pool() const noexcept;

	v<map_type> _map;
	storage_type _storage;
	map_rw_mutex _map_mutex;
	storage_mutex _storage_mutex;
};

/**
 * Dafault constructor.
 */
template <typename T, typename MapTraits, typename Storage>
enumerable_thread_specific<T, MapTraits, Storage>::enumerable_thread_specific()
{
}

/**
 * Dafault constructor.
 */
template <typename T, typename MapTraits, typename Storage>
enumerable_thread_specific<T, MapTraits, Storage>::~enumerable_thread_specific()
{
	_map.get().~map_type();
}

/**
 * Returns data reference for the current thread.
 * For the new thread, element by reference will be default constructed.
 *
 * @return reference to value for the current thread
 */
template <typename T, typename MapTraits, typename Storage>
typename enumerable_thread_specific<T, MapTraits, Storage>::reference
enumerable_thread_specific<T, MapTraits, Storage>::local()
{
	bool exists;
	return local(exists);
}

/**
 * Returns data reference for the current thread.
 * For the new thread, element by reference will be default constructed.
 *
 * @return reference to value for the current thread
 */
template <typename T, typename MapTraits, typename Storage>
typename enumerable_thread_specific<T, MapTraits, Storage>::reference
enumerable_thread_specific<T, MapTraits, Storage>::local(bool &exists)
{
	const map_key_type key = std::this_thread::get_id();
	map_type &map = _map.get();

	/* read lock to try find key in map */
	map_scoped_lock lock(_map_mutex, false);
	typename map_type::const_iterator it = map.find(key);
	exists = it != map.cend();

	/* return value if thread already exists in map */
	if (exists) {
		return _storage[(*it).second];
	}
	/* if upgrade passed without releasing create and return bucket */
	/* if mutex released during upgrade we need to look for key again */
	if (!lock_traits::upgrade_to_writer(lock)) {
		it = map.find(key);
		exists = it != map.cend();
		/* return bucket if it's found */
		if (exists) {
			return _storage[(*it).second];
		}
	}
	/* create bucket if it's not found */
	map_value_type value = storage_emplace();
	map[key] = value;
	return _storage[value];
}

/**
 * Private helper function. Default constructs element. Captures mutex.
 *
 * @return value_type of map_type, that indexing the added item of storage
 */
template <typename T, typename MapTraits, typename Storage>
typename enumerable_thread_specific<T, MapTraits, Storage>::map_value_type
enumerable_thread_specific<T, MapTraits, Storage>::storage_emplace()
{
	_storage_mutex.lock();

	map_value_type value = _storage.size();
	_storage.emplace_back();

	_storage_mutex.unlock();
	return value;
}

/**
 * Removes all elements from the container.
 *
 * @post empty() == true
 */
template <typename T, typename MapTraits, typename Storage>
void
enumerable_thread_specific<T, MapTraits, Storage>::clear()
{
	_storage_mutex.lock();

	_map.get().clear();
	_storage.clear();

	_storage_mutex.unlock();
}

/**
 * Method that returns how many elements container stores now.
 *
 * @return number of elements in container.
 */
template <typename T, typename MapTraits, typename Storage>
typename enumerable_thread_specific<T, MapTraits, Storage>::size_type
enumerable_thread_specific<T, MapTraits, Storage>::size() const
{
	return _storage.size();
}

/**
 * Determines if container is empty or not.
 *
 * @return true if container is empty, false overwise.
 */
template <typename T, typename MapTraits, typename Storage>
bool
enumerable_thread_specific<T, MapTraits, Storage>::empty() const
{
	return _storage.empty();
}

/**
 * Returns an iterator to the beginning.
 *
 * @return iterator to the first element in the container.
 */
template <typename T, typename MapTraits, typename Storage>
typename enumerable_thread_specific<T, MapTraits, Storage>::iterator
enumerable_thread_specific<T, MapTraits, Storage>::begin()
{
	return _storage.begin();
}

/**
 * Returns an iterator to element after the last.
 *
 * @return iterator to the after last element in the container.
 */
template <typename T, typename MapTraits, typename Storage>
typename enumerable_thread_specific<T, MapTraits, Storage>::iterator
enumerable_thread_specific<T, MapTraits, Storage>::end()
{
	return _storage.end();
}

/**
 * Returns an const_iterator to the beginning.
 *
 * @return const_iterator to the first element in the container.
 */
template <typename T, typename MapTraits, typename Storage>
typename enumerable_thread_specific<T, MapTraits, Storage>::const_iterator
enumerable_thread_specific<T, MapTraits, Storage>::begin() const
{
	return _storage.begin();
}

/**
 * Returns an const_iterator to element after the last.
 *
 * @return const_iterator to the after last element in the container.
 */
template <typename T, typename MapTraits, typename Storage>
typename enumerable_thread_specific<T, MapTraits, Storage>::const_iterator
enumerable_thread_specific<T, MapTraits, Storage>::end() const
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
template <typename T, typename MapTraits, typename Storage>
pool_base
enumerable_thread_specific<T, MapTraits, Storage>::get_pool() const noexcept
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);
	return pool_base(pop);
}

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */

#endif
