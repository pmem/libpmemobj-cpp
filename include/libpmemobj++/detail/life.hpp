// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/**
 * @file
 * Functions for destroying arrays.
 */

#ifndef LIBPMEMOBJ_CPP_DESTROYER_HPP
#define LIBPMEMOBJ_CPP_DESTROYER_HPP

#include <cstddef>
#include <type_traits>
#include <utility>

#include <libpmemobj++/detail/array_traits.hpp>
#include <libpmemobj++/detail/integer_sequence.hpp>

namespace pmem
{

namespace detail
{

/*
 * Template for checking if T is not an array.
 */
template <typename T>
struct if_not_array {
	typedef T type;
};

/*
 * Template for checking if T is not an array.
 */
template <typename T>
struct if_not_array<T[]>;

/*
 * Template for checking if T is not an array.
 */
template <typename T, size_t N>
struct if_not_array<T[N]>;

/*
 * Template for checking if T is an array.
 */
template <typename T>
struct if_size_array;

/*
 * Template for checking if T is an array.
 */
template <typename T>
struct if_size_array<T[]>;

/*
 * Template for checking if T is an array.
 */
template <typename T, size_t N>
struct if_size_array<T[N]> {
	typedef T type[N];
};

/*
 * Calls object's constructor.
 *
 * Supports aggregate initialization since C++17
 */
template <typename T, typename... Args>
void
create(typename if_not_array<T>::type *ptr, Args &&... args)
{
#if __cpp_lib_is_aggregate
	if constexpr (std::is_aggregate_v<T>)
		new (static_cast<void *>(ptr)) T{std::forward<Args>(args)...};
	else
		new (static_cast<void *>(ptr)) T(std::forward<Args>(args)...);
#else
	new (static_cast<void *>(ptr)) T(std::forward<Args>(args)...);
#endif
}

/*
 * Recursively calls array's elements' constructors.
 */
template <typename T, typename... Args>
void
create(typename if_size_array<T>::type *ptr, Args &&... args)
{
	typedef typename detail::pp_array_type<T>::type I;
	enum { N = pp_array_elems<T>::elems };

	for (std::size_t i = 0; i < N; ++i)
		create<I>(&(*ptr)[i], std::forward<Args>(args)...);
}

/*
 * Calls the objects constructor.
 *
 * Unpacks the tuple to get constructor's parameters.
 */
template <typename T, size_t... Indices, typename Tuple>
void
create_from_tuple(void *ptr, index_sequence<Indices...>, Tuple tuple)
{
	new (ptr) T(std::get<Indices>(std::move(tuple))...);
}

/*
 * C-style function which calls T constructor with arguments packed in a tuple.
 *
 * The arg is a tuple containing constructor parameters.
 */
template <typename T, typename Tuple, typename... Args>
int
c_style_construct(void *ptr, void *arg)
{
	auto *arg_pack = static_cast<Tuple *>(arg);

	typedef typename make_index_sequence<Args...>::type index;
	try {
		create_from_tuple<T>(ptr, index(), std::move(*arg_pack));
	} catch (...) {
		return -1;
	}

	return 0;
}

/*
 * Calls object's destructor.
 */
template <typename T,
	  typename = typename std::enable_if<
		  !std::is_trivially_destructible<T>::value>::type>
void
destroy(typename if_not_array<T>::type &arg)
{
	arg.~T();
}

/*
 * Don't call destructors for POD types.
 */
template <typename T, typename dummy = void,
	  typename = typename std::enable_if<
		  std::is_trivially_destructible<T>::value>::type>
void
destroy(typename if_not_array<T>::type &)
{
}

/*
 * Recursively calls array's elements' destructors.
 */
template <typename T>
void
destroy(typename if_size_array<T>::type &arg)
{
	typedef typename detail::pp_array_type<T>::type I;
	enum { N = pp_array_elems<T>::elems };

	for (std::size_t i = 0; i < N; ++i)
		destroy<I>(arg[N - 1 - i]);
}

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_DESTROYER_HPP */
