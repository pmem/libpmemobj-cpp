// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2019, Intel Corporation */

/**
 * @file
 * Libpmemobj C++ utils.
 */
#ifndef LIBPMEMOBJ_CPP_UTILS_HPP
#define LIBPMEMOBJ_CPP_UTILS_HPP

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj/base.h>

namespace pmem
{

namespace obj
{

/**
 * Retrieve pool handle for the given pointer.
 *
 * @param[in] that pointer to an object from a persistent memory pool.
 *
 * @return handle to the pool containing the object.
 *
 * @throw pool_error if the given pointer does not belong to an open pool.
 */
template <typename T>
inline pool_base
pool_by_vptr(const T *that)
{
	auto pop = pmemobj_pool_by_ptr(that);
	if (!pop)
		throw pmem::pool_error("Object not in an open pool.");

	return pool_base(pop);
}

/**
 * Retrieve pool handle for the given persistent_ptr.
 *
 * @param[in] ptr pointer to an object from a persistent memory pool.
 *
 * @return handle to the pool containing the object.
 *
 * @throw pool_error if the given pointer does not belong to an open pool.
 */
template <typename T>
inline pool_base
pool_by_pptr(const persistent_ptr<T> ptr)
{
	auto pop = pmemobj_pool_by_oid(ptr.raw());
	if (!pop)
		throw pmem::pool_error("Object not in an open pool.");

	return pool_base(pop);
}

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_UTILS_HPP */
