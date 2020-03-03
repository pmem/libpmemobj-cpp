// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

/**
 * @file
 * temp_value template class for caching objects.
 */

#ifndef LIBPMEMOBJ_CPP_TEMP_VALUE_HPP
#define LIBPMEMOBJ_CPP_TEMP_VALUE_HPP

#include <libpmemobj++/make_persistent.hpp>

namespace pmem
{

namespace detail
{

/**
 * Suggested maximum size of stack allocation for caching objects is 64kB.
 * It can be changed by user by setting LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE to
 * custom value.
 */
#ifndef LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE
#define LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE 64 * (1 << 10)
#endif

/**
 * Template class for caching objects based on constructor's variadic template
 * arguments and LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE. It has two
 * specializations: for noexcept constructors and for throwing constructors.
 */
template <typename T, bool NoExcept, typename Enable = void>
struct temp_value;

/**
 * Specialization for non-throwing constructors and objects smaller than
 * LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE bytes. Constructs and stores value in
 * underlying field.
 */
template <typename T, bool NoExcept>
struct temp_value<
	T, NoExcept,
	typename std::enable_if<NoExcept &&
				(sizeof(T) <
				 LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE)>::type> {
	template <typename... Args>
	temp_value(Args &&... args) noexcept : t(std::forward<Args>(args)...)
	{
	}

	T &
	get() noexcept
	{
		return t;
	}

	T t;
};

/**
 * Specialization for throwing constructors or objects greater than or equal to
 * LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE bytes. Constructs and stores value in
 * underlying field in pmem.
 */
template <typename T, bool NoExcept>
struct temp_value<
	T, NoExcept,
	typename std::enable_if<!NoExcept ||
				(sizeof(T) >=
				 LIBPMEMOBJ_CPP_MAX_STACK_ALLOC_SIZE)>::type> {
	template <typename... Args>
	temp_value(Args &&... args)
	{
		ptr = pmem::obj::make_persistent<T>(
			std::forward<Args>(args)...);
	}

	~temp_value()
	{
		pmem::obj::delete_persistent<T>(ptr);
	}

	T &
	get()
	{
		return *ptr;
	}

	pmem::obj::persistent_ptr<T> ptr;
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_TEMP_VALUE_HPP */
