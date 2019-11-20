/*
 * Copyright 2018-2019, Intel Corporation
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
 * Volatile resides on pmem property template.
 */

#ifndef LIBPMEMOBJ_CPP_V_HPP
#define LIBPMEMOBJ_CPP_V_HPP

#include <memory>
#include <shared_mutex>
#include <tuple>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>

#include <shared_mutex>
#include <unordered_map>

namespace pmem
{

namespace obj
{

namespace experimental
{

class v2 {
public:
	v2()
	{
	}

	v2(const v2 &) = delete;

	v2 &operator=(const v2 &) = delete;

	template <typename T>
	T &
	get()
	{
		return get<T>(pmemobj_oid(this));
	}

	template <typename T>
	T *
	get_if_exists()
	{
		return get_if_exists<T>(pmemobj_oid(this));
	}

	~v2()
	{
		destroy(pmemobj_oid(this));
	}

	template <typename T>
	T *
	get_if_exists(PMEMoid oid)
	{
		auto &map = get_map();

		{
			std::shared_lock<rwlock_type> lock(get_rwlock());
			auto it = map.find(oid);
			if (it != map.end())
				return static_cast<T *>(it->second.get());
			else
				return nullptr;
		}
	}

	template <typename T>
	T &
	get(PMEMoid oid)
	{
		auto &map = get_map();

		auto element = get_if_exists<T>(oid);
		if (element)
			return *element;

		if (pmemobj_tx_stage() == TX_STAGE_WORK)
			throw pmem::transaction_scope_error(
				"get() cannot be called in a transaction");

		// XXX: if we had on_free callback we can also call this in a
		// transaction just call destroy(oid) in on_free

		{
			std::unique_lock<rwlock_type> lock(get_rwlock());

			auto deleter = [](void const *data) {
				T const *p = static_cast<T const *>(data);
				delete p;
			};

			std::unique_ptr<
				void,
				std::add_pointer<void(const void *)>::type>
				ptr(new T, deleter);

			auto it = map.emplace(std::piecewise_construct,
					      std::forward_as_tuple(oid),
					      std::forward_as_tuple(new T,
								    deleter))
					  .first;

			/*
			 * emplace() could failed if another thread created
			 * the element when we dropped read and acquire write
			 * lock, in that case it will just point to existing
			 * element.
			 */
			return *static_cast<T *>(it->second.get());
		}
	}

	static void
	destroy(PMEMoid oid)
	{
		std::unique_lock<rwlock_type> lock(get_rwlock());
		get_map().erase(oid);

		// register_on_commit([]{std::unique_lock<rwlock_type>
		// lock(get_rwlock()); get_map().erase(oid)});
	}

	static void
	clear_from_pool(uint64_t pool_id)
	{
		std::unique_lock<rwlock_type> lock(get_rwlock());
		auto &map = get_map();

		for (auto it = map.begin(); it != map.end();) {
			if (it->first.pool_uuid_lo == pool_id)
				it = map.erase(it);
			else
				++it;
		}
	}

private:
	struct pmemoid_hash {
		std::size_t
		operator()(const PMEMoid &oid) const
		{
			return oid.pool_uuid_lo + oid.off;
		}
	};

	struct pmemoid_equal_to {
		bool
		operator()(const PMEMoid &lhs, const PMEMoid &rhs) const
		{
			return lhs.pool_uuid_lo == rhs.pool_uuid_lo &&
				lhs.off == rhs.off;
		}
	};

	// XXX: it would be better to have a container which does not reallocate
	// elements - we could have lock-free read.

	// XXX: could we make objects cacheable? If in map, we would hold T**
	// and return T* to the user, he could cache the pointer (in
	// runtime_initialize for example). destroy(PMEMoid) would not remove
	// entry from hashmap, instead it would only do: *T = nullptr. Every
	// reference to cached value would first check if *T is nullptr or not -
	// if it is, it could just call get() The hashmap would ONLY be cleared
	// at pool close() (+ in on_free callback)?
	using map_type = std::unordered_map<
		PMEMoid,
		std::unique_ptr<void,
				std::add_pointer<void(const void *)>::type>,
		pmemoid_hash, pmemoid_equal_to>;
	using rwlock_type = std::shared_timed_mutex;

	static map_type &
	get_map()
	{
		static map_type map;
		return map;
	}

	static rwlock_type &
	get_rwlock()
	{
		static rwlock_type rwlock;
		return rwlock;
	}
};

/**
 * pmem::obj::experimental::v - volatile resides on pmem class.
 *
 * v class is a property-like template class that has to be used for all
 * volatile variables that reside on persistent memory.
 * This class ensures that the enclosed type is always properly initialized by
 * always calling the class default constructor exactly once per instance of the
 * application.
 * This class has 8 bytes of storage overhead.
 * @snippet doc_snippets/v.cpp v_property_example
 */
template <typename T>
class v {
public:
	static_assert(std::is_default_constructible<T>::value,
		      "Type T must be default constructible");

	/**
	 * Defaulted constructor.
	 */
	v() noexcept : vlt{0}
	{
	}

	/**
	 * Destructor.
	 */
	~v()
	{
		/* Destructor of val should NOT be called */
	}

	/**
	 * Assignment operator.
	 */
	v &
	operator=(const T &rhs)
	{
		/* make sure object is initialized */
		(void)get();

		val = rhs;

		return *this;
	}

	/**
	 * Assignment operator.
	 */
	v &
	operator=(v &rhs)
	{
		return *this = rhs.get();
	}

	/**
	 * Converting assignment operator from a different v<>.
	 *
	 * Available only for convertible types.
	 */
	template <typename Y,
		  typename = typename std::enable_if<
			  std::is_convertible<Y, T>::value>::type>
	v &
	operator=(v<Y> &rhs)
	{
		return *this = rhs.get();
	}

	/**
	 * Retrieves reference to the object.
	 *
	 * @param[in] args forwarded to objects constructor. If object was
	 * constructed earlier during application lifetime (even with different
	 * arguments) no constructor is called.
	 *
	 * @return a reference to the object.
	 */
	template <typename... Args>
	T &
	get(Args &&... args) noexcept
	{
		auto arg_pack =
			std::forward_as_tuple(std::forward<Args>(args)...);

		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		if (pop == NULL)
			return this->val;

		T *value = static_cast<T *>(pmemobj_volatile(
			pop, &this->vlt, &this->val, sizeof(T),
			pmem::detail::c_style_construct<T, decltype(arg_pack),
							Args...>,
			static_cast<void *>(&arg_pack)));

		return *value;
	}

	/**
	 * Retrieves reference to the object.
	 *
	 * If object was not constructed (e.g. using get()) return value is
	 * unspecified.
	 *
	 * @return a reference to the object.
	 */
	T &
	unsafe_get()
	{
		return val;
	}

	/**
	 * Conversion operator back to the underlying type.
	 */
	operator T &() noexcept
	{
		return this->get();
	}

	/**
	 * Swaps two v objects of the same type.
	 */
	void
	swap(v &other)
	{
		std::swap(get(), other.get());
	}

private:
	struct pmemvlt vlt;

	/*
	 * Normally C++ requires all class members to be constructed during
	 * enclosing type construction. Holding a value inside of a union allows
	 * to bypass this requirement. val is only constructed by call to get().
	 */
	union {
		T val;
	};
};

/**
 * Swaps two v objects of the same type.
 *
 * Non-member swap function as required by Swappable concept.
 * en.cppreference.com/w/cpp/concept/Swappable
 */
template <class T>
inline void
swap(v<T> &a, v<T> &b)
{
	a.swap(b);
}

} /* namespace experimental */

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_V_HPP */
