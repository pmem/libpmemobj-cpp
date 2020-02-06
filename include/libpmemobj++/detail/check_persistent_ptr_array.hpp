// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2018, Intel Corporation */

/**
 * @file
 * Compile time type check for make_persistent.
 */

#ifndef LIBPMEMOBJ_CPP_CHECK_PERSISTENT_PTR_ARRAY_HPP
#define LIBPMEMOBJ_CPP_CHECK_PERSISTENT_PTR_ARRAY_HPP

#include <cstddef>

#include <libpmemobj++/persistent_ptr.hpp>

namespace pmem
{

namespace detail
{

/*
 * Typedef checking if given type is not an array.
 */
template <typename T>
struct pp_if_not_array {
	typedef obj::persistent_ptr<T> type;
};

/*
 * Typedef checking if given type is not an array.
 */
template <typename T>
struct pp_if_not_array<T[]> {
};

/*
 * Typedef checking if given type is not an array.
 */
template <typename T, size_t N>
struct pp_if_not_array<T[N]> {
};

/*
 * Typedef checking if given type is an array.
 */
template <typename T>
struct pp_if_array;

/*
 * Typedef checking if given type is an array.
 */
template <typename T>
struct pp_if_array<T[]> {
	typedef obj::persistent_ptr<T[]> type;
};

/*
 * Typedef checking if given type is an array.
 */
template <typename T>
struct pp_if_size_array;

/*
 * Typedef checking if given type is an array.
 */
template <typename T, size_t N>
struct pp_if_size_array<T[N]> {
	typedef obj::persistent_ptr<T[N]> type;
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_CHECK_PERSISTENT_PTR_ARRAY_HPP */
