// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2019, Intel Corporation */

/**
 * @file
 * Atomic persistent_ptr allocation functions for arrays. The typical usage
 * examples would be:
 * @snippet doc_snippets/make_persistent.cpp make_array_atomic_example
 */

#ifndef LIBPMEMOBJ_CPP_MAKE_PERSISTENT_ARRAY_ATOMIC_HPP
#define LIBPMEMOBJ_CPP_MAKE_PERSISTENT_ARRAY_ATOMIC_HPP

#include <libpmemobj++/allocation_flag.hpp>
#include <libpmemobj++/detail/array_traits.hpp>
#include <libpmemobj++/detail/check_persistent_ptr_array.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/make_atomic_impl.hpp>
#include <libpmemobj++/detail/variadic.hpp>
#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj/atomic_base.h>

namespace pmem
{

namespace obj
{

/**
 * Atomically allocate an array of objects.
 *
 * This function can be used to atomically allocate an array of objects.
 * Cannot be used for simple objects. Do *NOT* use this inside transactions, as
 * it might lead to undefined behavior in the presence of transaction aborts.
 *
 * @param[in,out] pool the pool from which the object will be allocated.
 * @param[in,out] ptr the persistent pointer to which the allocation
 *	will take place.
 * @param[in] N the number of array elements.
 * @param[in] flag affects behaviour of allocator
 *
 * @throw std::bad_alloc on allocation failure.
 */
template <typename T>
void
make_persistent_atomic(
	pool_base &pool, typename detail::pp_if_array<T>::type &ptr,
	std::size_t N,
	allocation_flag_atomic flag = allocation_flag_atomic::none())
{
	typedef typename detail::pp_array_type<T>::type I;

	auto ret = pmemobj_xalloc(pool.handle(), ptr.raw_ptr(), sizeof(I) * N,
				  detail::type_num<I>(), flag.value,
				  &detail::array_constructor<I>,
				  static_cast<void *>(&N));

	if (ret != 0)
		throw std::bad_alloc();
}

/**
 * Atomically allocate an array of objects.
 *
 * This function can be used to atomically allocate an array of objects.
 * Cannot be used for simple objects. Do *NOT* use this inside transactions, as
 * it might lead to undefined behavior in the presence of transaction aborts.
 *
 * @param[in,out] pool the pool from which the object will be allocated.
 * @param[in,out] ptr the persistent pointer to which the allocation
 *	will take place.
 * @param[in] flag affects behaviour of allocator
 *
 * @throw std::bad_alloc on allocation failure.
 */
template <typename T>
void
make_persistent_atomic(
	pool_base &pool, typename detail::pp_if_size_array<T>::type &ptr,
	allocation_flag_atomic flag = allocation_flag_atomic::none())
{
	typedef typename detail::pp_array_type<T>::type I;
	std::size_t N = detail::pp_array_elems<T>::elems;

	auto ret = pmemobj_xalloc(pool.handle(), ptr.raw_ptr(), sizeof(I) * N,
				  detail::type_num<I>(), flag.value,
				  &detail::array_constructor<I>,
				  static_cast<void *>(&N));

	if (ret != 0)
		throw std::bad_alloc();
}

/**
 * Atomically deallocate an array of objects.
 *
 * There is no way to atomically destroy an object. Any object specific
 * cleanup must be performed elsewhere. Do *NOT* use this inside transactions,
 * as it might lead to undefined behavior in the presence of transaction aborts.
 *
 * param[in,out] ptr the persistent_ptr whose pointee is to be
 * deallocated.
 */
template <typename T>
void
delete_persistent_atomic(typename detail::pp_if_array<T>::type &ptr,
			 std::size_t)
{
	if (ptr == nullptr)
		return;

	/* we CAN'T call destructor */
	pmemobj_free(ptr.raw_ptr());
}

/**
 * Atomically deallocate an array of objects.
 *
 * There is no way to atomically destroy an object. Any object specific
 * cleanup must be performed elsewhere. Do *NOT* use this inside transactions,
 * as it might lead to undefined behavior in the presence of transaction aborts.
 *
 * param[in,out] ptr the persistent_ptr whose pointee is to be deallocated.
 */
template <typename T>
void
delete_persistent_atomic(typename detail::pp_if_size_array<T>::type &ptr)
{
	if (ptr == nullptr)
		return;

	/* we CAN'T call destructor */
	pmemobj_free(ptr.raw_ptr());
}

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_MAKE_PERSISTENT_ARRAY_ATOMIC_HPP */
