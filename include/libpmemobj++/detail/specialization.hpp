// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2018, Intel Corporation */

/**
 * @file
 * Helper template for persistent ptr specialization.
 *
 * Based on Boost library smart_ptr implementation.
 */

#ifndef LIBPMEMOBJ_CPP_SPECIALIZATION_HPP
#define LIBPMEMOBJ_CPP_SPECIALIZATION_HPP

#include <memory>

namespace pmem
{

namespace detail
{
/* smart pointer specialization */

template <typename T>
struct sp_element {
	typedef T type;
};

template <typename T>
struct sp_element<T[]> {
	typedef T type;
};

template <typename T, std::size_t N>
struct sp_element<T[N]> {
	typedef T type;
};

/* sp_dereference is a return type of operator* */

template <typename T>
struct sp_dereference {
	typedef T &type;
};

template <>
struct sp_dereference<void> {
	typedef void type;
};

template <>
struct sp_dereference<void const> {
	typedef void type;
};

template <>
struct sp_dereference<void volatile> {
	typedef void type;
};

template <>
struct sp_dereference<void const volatile> {
	typedef void type;
};

template <typename T>
struct sp_dereference<T[]> {
	typedef void type;
};

template <typename T, std::size_t N>
struct sp_dereference<T[N]> {
	typedef void type;
};

/* sp_member_access is a return type of operator-> */

template <typename T>
struct sp_member_access {
	typedef T *type;
};

template <typename T>
struct sp_member_access<T[]> {
	typedef void type;
};

template <typename T, std::size_t N>
struct sp_member_access<T[N]> {
	typedef void type;
};

/* sp_array_access is a return type of operator[] */

template <typename T>
struct sp_array_access {
	typedef T &type;
};

template <>
struct sp_array_access<void> {
	typedef struct does_not_exist {
	} & type;
};

template <typename T>
struct sp_array_access<T[]> {
	typedef T &type;
};

template <typename T, std::size_t N>
struct sp_array_access<T[N]> {
	typedef T &type;
};

/* sp_extent is used for operator[] index checking */

template <typename T>
struct sp_extent {
	enum _vt { value = 0 };
};

template <typename T, std::size_t N>
struct sp_extent<T[N]> {
	enum _vt { value = N };
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_SPECIALIZATION_HPP */
