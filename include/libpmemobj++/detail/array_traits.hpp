// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2018, Intel Corporation */

/**
 * @file
 * Common array traits.
 */

#ifndef LIBPMEMOBJ_CPP_ARRAY_TRAITS_HPP
#define LIBPMEMOBJ_CPP_ARRAY_TRAITS_HPP

#include <cstddef>

namespace pmem
{

namespace detail
{

/*
 * Returns the number of array elements.
 */
template <typename T>
struct pp_array_elems {
	enum { elems = 1 };
};

/*
 * Returns the number of array elements.
 */
template <typename T, size_t N>
struct pp_array_elems<T[N]> {
	enum { elems = N };
};

/*
 * Returns the type of elements in an array.
 */
template <typename T>
struct pp_array_type;

/*
 * Returns the type of elements in an array.
 */
template <typename T>
struct pp_array_type<T[]> {
	typedef T type;
};

/*
 * Returns the type of elements in an array.
 */
template <typename T, size_t N>
struct pp_array_type<T[N]> {
	typedef T type;
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_ARRAY_TRAITS_HPP */
