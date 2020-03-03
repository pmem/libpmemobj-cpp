// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018, Intel Corporation */

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
