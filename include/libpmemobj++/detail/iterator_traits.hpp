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
 * Common iterator traits.
 */

#ifndef LIBPMEMOBJ_CPP_ITERATOR_TRAITS_HPP
#define LIBPMEMOBJ_CPP_ITERATOR_TRAITS_HPP

#include <iterator>

namespace pmem
{

namespace detail
{

template <typename T, typename U, typename C = void>
struct has_iterator_category_convertible_to : std::false_type {
};

template <typename T, typename U>
struct has_iterator_category_convertible_to<
	T, U,
	typename std::enable_if<std::is_convertible<
		typename std::iterator_traits<T>::iterator_category,
		U>::value>::type> : std::true_type {
};

/**
 * Type trait to determine if a given parameter type satisfies requirements of
 * OutputIterator.
 */
template <typename T>
struct is_output_iterator
    : public has_iterator_category_convertible_to<T, std::output_iterator_tag> {
};

/**
 * Type trait to determine if a given parameter type satisfies requirements of
 * InputIterator.
 */
template <typename T>
struct is_input_iterator
    : public has_iterator_category_convertible_to<T, std::input_iterator_tag> {
};

/**
 * Type trait to determine if a given parameter type satisfies requirements of
 * ForwardIterator.
 */
template <typename T>
struct is_forward_iterator
    : public has_iterator_category_convertible_to<T,
						  std::forward_iterator_tag> {
};

/**
 * Type trait to determine if a given parameter type satisfies requirements of
 * BidirectionalIterator.
 */
template <typename T>
struct is_bidirectional_iterator : public has_iterator_category_convertible_to<
					   T, std::bidirectional_iterator_tag> {
};

/**
 * Type trait to determine if a given parameter type satisfies requirements of
 * RandomAccessIterator.
 */
template <typename T>
struct is_random_access_iterator : public has_iterator_category_convertible_to<
					   T, std::random_access_iterator_tag> {
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_ITERATOR_TRAITS_HPP */
