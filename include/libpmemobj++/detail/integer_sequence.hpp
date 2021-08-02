// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2021, Intel Corporation */

/**
 * @file
 * Create c++14 style index sequence.
 */

#ifndef LIBPMEMOBJ_CPP_INTEGER_SEQUENCE_HPP
#define LIBPMEMOBJ_CPP_INTEGER_SEQUENCE_HPP

#include <cstddef>
#include <type_traits>

namespace pmem
{

namespace detail
{

/*
 * Base index type template.
 */
template <typename T, T...>
struct integer_sequence {
};

/*
 * Size_t specialization of the integer sequence.
 */
template <size_t... Indices>
using index_sequence = integer_sequence<size_t, Indices...>;

template <typename T, T N, typename Enable, T... I>
struct make_integer_seq_impl;

/*
 * Helper for make_integer_sequence, specialization for N == 0 (end of
 * recursion).
 */
template <typename T, T N, T... I>
struct make_integer_seq_impl<T, N, typename std::enable_if<N == 0>::type,
			     I...> {
	using type = integer_sequence<T, I...>;
};

/*
 * Helper for make_integer_sequence, base case.
 */
template <typename T, T N, T... I>
struct make_integer_seq_impl<T, N, typename std::enable_if<N != 0>::type,
			     I...> {
	using type = typename make_integer_seq_impl<T, N - 1, void, N - 1,
						    I...>::type;
};

/*
 * C++11 implementation of std::make_integer_sequence.
 */
template <typename T, T N>
using make_integer_sequence = typename make_integer_seq_impl<T, N, void>::type;

/*
 * C++11 implementation of std::make_index_sequence.
 */
template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

/*
 * C++11 implementation of std::index_sequence_for.
 *
 * A helper alias template to convert any type parameter pack into an index
 * sequence of the same length.
 */
template <class... Types>
using index_sequence_for = make_index_sequence<sizeof...(Types)>;

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_INTEGER_SEQUENCE_HPP */
