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
 * Implementation details of atomic allocation and construction.
 */

#ifndef LIBPMEMOBJ_CPP_MAKE_ATOMIC_IMPL_HPP
#define LIBPMEMOBJ_CPP_MAKE_ATOMIC_IMPL_HPP

#include <cstddef>
#include <new>

#include <libpmemobj++/detail/array_traits.hpp>
#include <libpmemobj++/detail/integer_sequence.hpp>
#include <libpmemobj++/detail/life.hpp>

namespace pmem
{

namespace detail
{

/*
 * C-style function called by the allocator.
 *
 * The arg is a tuple containing constructor parameters.
 */
template <typename T, typename Tuple, typename... Args>
int
obj_constructor(PMEMobjpool *pop, void *ptr, void *arg)
{
	auto ret = c_style_construct<T, Tuple, Args...>(ptr, arg);

	if (ret != 0)
		return -1;

	pmemobj_persist(pop, ptr, sizeof(T));

	return 0;
}

/*
 * Constructor used for atomic array allocations.
 *
 * Returns -1 if an exception was thrown during T's construction,
 * 0 otherwise.
 */
template <typename T>
int
array_constructor(PMEMobjpool *pop, void *ptr, void *arg)
{
	std::size_t N = *static_cast<std::size_t *>(arg);

	T *tptr = static_cast<T *>(ptr);
	try {
		for (std::size_t i = 0; i < N; ++i)
			detail::create<T>(tptr + i);
	} catch (...) {
		return -1;
	}

	pmemobj_persist(pop, ptr, sizeof(T) * N);

	return 0;
}

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_MAKE_ATOMIC_IMPL_HPP */
