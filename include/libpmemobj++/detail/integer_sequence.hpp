// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/**
 * @file
 * Create c++14 style index sequence.
 */

#ifndef LIBPMEMOBJ_CPP_INTEGER_SEQUENCE_HPP
#define LIBPMEMOBJ_CPP_INTEGER_SEQUENCE_HPP

#include <cstddef>

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

/*
 * Empty base class.
 *
 * Subject of empty base optimization.
 */
template <typename T, T I, typename... Types>
struct make_integer_seq_impl;

/*
 * Class ending recursive variadic template peeling.
 */
template <typename T, T I, T... Indices>
struct make_integer_seq_impl<T, I, integer_sequence<T, Indices...>> {
	typedef integer_sequence<T, Indices...> type;
};

/*
 * Recursively create index while peeling off the types.
 */
template <typename N, N I, N... Indices, typename T, typename... Types>
struct make_integer_seq_impl<N, I, integer_sequence<N, Indices...>, T,
			     Types...> {
	typedef typename make_integer_seq_impl<
		N, I + 1, integer_sequence<N, Indices..., I>, Types...>::type
		type;
};

/*
 * Make index sequence entry point.
 */
template <typename... Types>
using make_index_sequence =
	make_integer_seq_impl<size_t, 0, integer_sequence<size_t>, Types...>;

/*
 * A helper alias template to convert any type parameter pack into an index
 * sequence of the same length. Analog of std::index_sequence_for.
 */
template <class... Types>
using index_sequence_for = typename make_index_sequence<Types...>::type;

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_INTEGER_SEQUENCE_HPP */
