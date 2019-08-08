/*
 * Copyright 2019, Intel Corporation
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

#ifndef PMEMOBJ_TX_ALLOCATOR_HPP
#define PMEMOBJ_TX_ALLOCATOR_HPP

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj/tx_base.h>

namespace pmem
{
namespace obj
{
namespace experimental
{

template <typename T>
class tx_allocator {
public:
	using value_type = T;
	using size_type = size_t;
	using pointer = pmem::obj::persistent_ptr<T>;

	template <typename U>
	friend class tx_allocator;

	tx_allocator() = default;

	tx_allocator(const tx_allocator &) = default;

	tx_allocator(tx_allocator &&) = default;

	template <typename U>
	tx_allocator(const tx_allocator<U> &) noexcept
	{
	}

	template <typename U>
	tx_allocator(tx_allocator<U> &&) noexcept
	{
	}

	tx_allocator &operator=(const tx_allocator &) = default;

	template <typename U>
	tx_allocator &operator=(const tx_allocator<U> &) noexcept {};

	tx_allocator &operator=(tx_allocator &&) = default;

	template <typename U>
	tx_allocator &operator=(tx_allocator<U> &&) noexcept {};

	pointer
	allocate(size_type n) const
	{
		pointer result = pmemobj_tx_alloc(
			sizeof(value_type) * n, detail::type_num<value_type>());
		if (result == nullptr) {
			throw std::bad_alloc();
		}
		return result;
	}

	void
	deallocate(pointer p, size_type n) const
	{
		// TODO: Do we need exception if free failed
		pmemobj_tx_free(*p.raw_ptr());
	}
}; // class tx_allocator

template <typename U, typename V>
bool
operator==(const tx_allocator<U> &lhs, const tx_allocator<V> &rhs)
{
	return true;
}

template <typename U, typename V>
bool
operator!=(const tx_allocator<U> &lhs, const tx_allocator<V> &rhs)
{
	return false;
}

} // namespace experimental
} // namespace obj
} // namespace pmem

#endif // PMEMOBJ_TX_ALLOCATOR_HPP
