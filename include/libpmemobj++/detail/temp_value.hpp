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
