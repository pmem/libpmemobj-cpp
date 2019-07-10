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
 * Persistent_ptr transactional allocation functions for objects. The typical
 * usage examples would be:
 * @snippet doc_snippets/make_persistent.cpp make_example
 */

#ifndef LIBPMEMOBJ_CPP_MAKE_PERSISTENT_HPP
#define LIBPMEMOBJ_CPP_MAKE_PERSISTENT_HPP

#include <libpmemobj++/allocation_flag.hpp>
#include <libpmemobj++/detail/check_persistent_ptr_array.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/detail/variadic.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj/tx_base.h>

#include <new>
#include <utility>

namespace pmem
{

namespace obj
{

/**
 * Transactionally allocate and construct an object of type T.
 *
 * This function can be used to *transactionally* allocate an object.
 * Cannot be used for array types.
 *
 * @param[in] flag affects behaviour of allocator
 * @param[in,out] args a list of parameters passed to the constructor.
 *
 * @return persistent_ptr<T> on success
 *
 * @throw transaction_scope_error if called outside of an active
 * transaction
 * @throw transaction_alloc_error on transactional allocation failure.
 * @throw rethrow exception from T constructor
 */
template <typename T, typename... Args>
typename detail::pp_if_not_array<T>::type
make_persistent(allocation_flag flag, Args &&... args)
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"refusing to allocate memory outside of transaction scope");

	persistent_ptr<T> ptr =
		pmemobj_tx_xalloc(sizeof(T), detail::type_num<T>(), flag.value);

	if (ptr == nullptr) {
		if (errno == ENOMEM)
			throw pmem::transaction_out_of_memory(
				"Failed to allocate persistent memory object")
				.with_pmemobj_errormsg();
		else
			throw pmem::transaction_alloc_error(
				"Failed to allocate persistent memory object")
				.with_pmemobj_errormsg();
	}

	detail::create<T, Args...>(ptr.get(), std::forward<Args>(args)...);

	return ptr;
}

/**
 * Transactionally allocate and construct an object of type T.
 *
 * This function can be used to *transactionally* allocate an object.
 * Cannot be used for array types.
 *
 * @param[in,out] args a list of parameters passed to the constructor.
 *
 * @return persistent_ptr<T> on success
 *
 * @throw transaction_scope_error if called outside of an active
 * transaction
 * @throw transaction_alloc_error on transactional allocation failure.
 * @throw rethrow exception from T constructor
 */
template <typename T, typename... Args>
typename std::enable_if<
	!detail::is_first_arg_same<allocation_flag, Args...>::value,
	typename detail::pp_if_not_array<T>::type>::type
make_persistent(Args &&... args)
{
	return make_persistent<T>(allocation_flag::none(),
				  std::forward<Args>(args)...);
}

/**
 * Transactionally free an object of type T held in a persistent_ptr.
 *
 * This function can be used to *transactionally* free an object. Calls the
 * object's destructor before freeing memory. Cannot be used for array
 * types.
 *
 * @param[in,out] ptr persistent pointer to an object that is not an
 * array.
 *
 * @throw transaction_scope_error if called outside of an active
 * transaction
 * @throw transaction_free_error on transactional free failure.
 */
template <typename T>
void
delete_persistent(typename detail::pp_if_not_array<T>::type ptr)
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"refusing to free memory outside of transaction scope");

	if (ptr == nullptr)
		return;

	/*
	 * At this point, everything in the object should be tracked
	 * and reverted on transaction abort.
	 */
	detail::destroy<T>(*ptr);

	if (pmemobj_tx_free(*ptr.raw_ptr()) != 0)
		throw pmem::transaction_free_error(
			"failed to delete persistent memory object")
			.with_pmemobj_errormsg();
}

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_MAKE_PERSISTENT_HPP */
