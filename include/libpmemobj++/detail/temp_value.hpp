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

#ifndef LIBPMEMOBJ_CPP_TEMP_VAULE_HPP
#define LIBPMEMOBJ_CPP_TEMP_VAULE_HPP

#include <libpmemobj++/make_persistent.hpp>

namespace pmem
{

namespace detail
{

/**
 * Template class for caching objects based on constructor's variadic template
 * arguments. It have two specializations: for noexcept constructors and for
 * throwing constructors.
 */
template <typename T, bool NoExcept, typename Enable = void>
struct temp_value;

/**
 * Specialization for no throwing constructors. Constructs and stores value in
 * underlying field.
 */
template <typename T, bool NoExcept>
struct temp_value<T, NoExcept, typename std::enable_if<NoExcept>::type> {
	template <typename... Args>
	temp_value(Args &&... args) : t(std::forward<Args>(args)...)
	{
	}

	T &
	get()
	{
		return t;
	}

	T t;
};

/**
 * Specialization for throwing constructors. Constructs and stores value in
 * underlying field in pmem.
 */
template <typename T, bool NoExcept>
struct temp_value<T, NoExcept, typename std::enable_if<!NoExcept>::type> {
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

#endif /* LIBPMEMOBJ_CPP_TEMP_VAULE_HPP */
