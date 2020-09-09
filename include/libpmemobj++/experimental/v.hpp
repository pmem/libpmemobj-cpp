// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/**
 * @file
 * Volatile residing on pmem property template.
 */

#ifndef LIBPMEMOBJ_CPP_V_HPP
#define LIBPMEMOBJ_CPP_V_HPP

#include <memory>
#include <tuple>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>

namespace pmem
{

namespace obj
{

namespace experimental
{

/**
 * Volatile residing on pmem class.
 *
 * v class is a property-like template class that has to be used for all
 * volatile variables that reside on persistent memory.
 * This class ensures that the enclosed type is always properly initialized by
 * always calling the class default constructor exactly once per instance of the
 * application.
 *
 * This class has 8 bytes of storage overhead.
 *
 * Example usage:
 * @snippet v/v.cpp v_property_example
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
