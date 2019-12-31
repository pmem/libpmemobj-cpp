/*
 * Copyright 2016-2019, Intel Corporation
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
 * Persistent_ptr allocation functions for arrays. The typical usage examples
 * would be:
 * @snippet doc_snippets/make_persistent.cpp make_array_example
 */

#ifndef LIBPMEMOBJ_CPP_MAKE_PERSISTENT_ARRAY_HPP
#define LIBPMEMOBJ_CPP_MAKE_PERSISTENT_ARRAY_HPP

#include <libpmemobj++/allocation_flag.hpp>
#include <libpmemobj++/detail/array_traits.hpp>
#include <libpmemobj++/detail/check_persistent_ptr_array.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/detail/variadic.hpp>
#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj/tx_base.h>

#include <cassert>
#include <limits>

namespace pmem
{

namespace obj
{

/**
 * Transactionally allocate and construct an array of objects of type T.
 *
 * This function can be used to *transactionally* allocate an array.
 * This overload only participates in overload resolution if T is an array.
 *
 * @param[in] N the number of array elements.
 * @param[in] flag affects behaviour of allocator
 *
 * @return persistent_ptr<T[]> on success
 *
 * @throw transaction_scope_error if called outside of an active
 * transaction
 * @throw transaction_alloc_error on transactional allocation failure.
 * @throw rethrow exception from T constructor
 */
template <typename T>
typename detail::pp_if_array<T>::type
make_persistent(std::size_t N, allocation_flag flag = allocation_flag::none())
{
	typedef typename detail::pp_array_type<T>::type I;

	/*
	 * Allowing N greater than ptrdiff_t max value would cause problems
	 * with accessing array and calculating address difference between two
	 * elements placed further apart than ptrdiff_t max value
	 */
	assert(N <=
	       static_cast<std::size_t>(std::numeric_limits<ptrdiff_t>::max()));

	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"refusing to allocate memory outside of transaction scope");

	persistent_ptr<T> ptr = pmemobj_tx_xalloc(
		sizeof(I) * N, detail::type_num<I>(), flag.value);

	if (ptr == nullptr) {
		if (errno == ENOMEM)
			throw pmem::transaction_out_of_memory(
				"Failed to allocate persistent memory array")
				.with_pmemobj_errormsg();
		else
			throw pmem::transaction_alloc_error(
				"Failed to allocate persistent memory array")
				.with_pmemobj_errormsg();
	}

	/*
	 * cache raw pointer to data - using persistent_ptr.get() in a loop
	 * is expensive.
	 */
	auto data = ptr.get();

	/*
	 * When an exception is thrown from one of the constructors
	 * we don't perform any cleanup - i.e. we don't call destructors
	 * (unlike new[] operator), we only rely on transaction abort.
	 * This approach was taken to ensure consistent behaviour for
	 * case when transaction is aborted after make_persistent completes and
	 * we have no way to call destructors.
	 */
	for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(N); ++i)
		detail::create<I>(data + i);

	return ptr;
}

/**
 * Transactionally allocate and construct an array of objects of type T.
 *
 * This function can be used to *transactionally* allocate an array.
 * This overload only participates in overload resolution if T is an array.
 *
 * @param[in] flag affects behaviour of allocator
 *
 * @return persistent_ptr<T[N]> on success
 *
 * @throw transaction_scope_error if called outside of an active
 * transaction
 * @throw transaction_alloc_error on transactional allocation failure.
 * @throw rethrow exception from T constructor
 */
template <typename T>
typename detail::pp_if_size_array<T>::type
make_persistent(allocation_flag flag = allocation_flag::none())
{
	typedef typename detail::pp_array_type<T>::type I;
	enum { N = detail::pp_array_elems<T>::elems };

	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"refusing to allocate memory outside of transaction scope");

	persistent_ptr<T> ptr = pmemobj_tx_xalloc(
		sizeof(I) * N, detail::type_num<I>(), flag.value);

	if (ptr == nullptr) {
		if (errno == ENOMEM)
			throw pmem::transaction_out_of_memory(
				"Failed to allocate persistent memory array")
				.with_pmemobj_errormsg();
		else
			throw pmem::transaction_alloc_error(
				"Failed to allocate persistent memory array")
				.with_pmemobj_errormsg();
	}

	/*
	 * cache raw pointer to data - using persistent_ptr.get() in a loop
	 * is expensive.
	 */
	auto data = ptr.get();

	/*
	 * When an exception is thrown from one of the constructors
	 * we don't perform any cleanup - i.e. we don't call destructors
	 * (unlike new[] operator), we only rely on transaction abort.
	 * This approach was taken to ensure consistent behaviour for
	 * case when transaction is aborted after make_persistent completes and
	 * we have no way to call destructors.
	 */
	for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(N); ++i)
		detail::create<I>(data + i);

	return ptr;
}

/**
 * Transactionally free an array of objects of type T held
 * in a persistent_ptr.
 *
 * This function can be used to *transactionally* free an array of
 * objects. Calls the objects' destructors before freeing memory.
 * This overload only participates in overload resolution if T is an array.
 *
 * @param[in,out] ptr persistent pointer to an array of objects.
 * @param[in] N the size of the array.
 *
 * @throw transaction_scope_error if called outside of an active
 * transaction
 * @throw transaction_free_error on transactional free failure.
 */
template <typename T>
void
delete_persistent(typename detail::pp_if_array<T>::type ptr, std::size_t N)
{
	typedef typename detail::pp_array_type<T>::type I;

	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"refusing to free memory outside of transaction scope");

	if (ptr == nullptr)
		return;

	/*
	 * cache raw pointer to data - using persistent_ptr.get() in a loop
	 * is expensive.
	 */
	auto data = ptr.get();

	for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(N); ++i)
		detail::destroy<I>(
			data[static_cast<std::ptrdiff_t>(N) - 1 - i]);

	if (pmemobj_tx_free(*ptr.raw_ptr()) != 0)
		throw pmem::transaction_free_error(
			"failed to delete persistent memory object")
			.with_pmemobj_errormsg();
}

/**
 * Transactionally free an array of objects of type T held
 * in a persistent_ptr.
 *
 * This function can be used to *transactionally* free an array of
 * objects. Calls the objects' destructors before freeing memory.
 * This overload only participates in overload resolution if T is an array.
 *
 * @param[in,out] ptr persistent pointer to an array of objects.
 *
 * @throw transaction_scope_error if called outside of an active
 * transaction
 * @throw transaction_free_error on transactional free failure.
 */
template <typename T>
void
delete_persistent(typename detail::pp_if_size_array<T>::type ptr)
{
	typedef typename detail::pp_array_type<T>::type I;
	enum { N = detail::pp_array_elems<T>::elems };

	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"refusing to free memory outside of transaction scope");

	if (ptr == nullptr)
		return;

	/*
	 * cache raw pointer to data - using persistent_ptr.get() in a loop
	 * is expensive.
	 */
	auto data = ptr.get();

	for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(N); ++i)
		detail::destroy<I>(
			data[static_cast<std::ptrdiff_t>(N) - 1 - i]);

	if (pmemobj_tx_free(*ptr.raw_ptr()) != 0)
		throw pmem::transaction_free_error(
			"failed to delete persistent memory object")
			.with_pmemobj_errormsg();
}

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_MAKE_PERSISTENT_ARRAY_HPP */
