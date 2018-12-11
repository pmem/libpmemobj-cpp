/*
 * Copyright 2018, Intel Corporation
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

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/volatile.hpp>

namespace pmem
{

namespace obj
{

namespace experimental
{

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
	using this_type = v<T>;

	template <typename N>
	friend class v;

public:
	/**
	 * Defaulted constructor.
	 */
	v() noexcept : vlt{0}
	{
	}

	/**
	 * Assignment operator.
	 */
	v &
	operator=(const v &rhs)
	{
		/* make sure object is initialized */
		(void)get();

		val = rhs.val;

		return *this;
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
	 * Converting assignment operator from a different v<>.
	 *
	 * Available only for convertible types.
	 */
	template <typename Y,
		  typename = typename std::enable_if<
			  std::is_convertible<Y, T>::value>::type>
	v &
	operator=(const v<Y> &rhs)
	{
		/* make sure object is initialized */
		(void)get();

		val = rhs.val;

		return *this;
	}

	/**
	 * Retrieves reference of the object.
	 *
	 * @return a reference to the object.
	 *
	 */
	T &
	get() noexcept
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);
		if (pop == NULL)
			return this->val;

		T *value = static_cast<T *>(pmemobj_volatile(
			pop, &this->vlt, &this->val, sizeof(T),
			pmem::detail::instantiate_volatile_object<T>, NULL));

		return *value;
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
		/* make sure object is initialized */
		(void)get();

		std::swap(this->val, other.val);
	}

private:
	struct pmemvlt vlt;
	T val;
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
