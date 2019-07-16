/*
 * Copyright 2018-2019, Intel Corporation
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

/*
 * list_wrapper.cpp -- wrapper for sharing tests between list-like data
 * structures
 */

/* if defined persistent vector */
#ifdef VECTOR

#include <libpmemobj++/experimental/vector.hpp>

template <typename T>
using container_t = pmem::obj::experimental::vector<T>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type size;
	typename container_t<T>::size_type capacity;
	pmem::obj::persistent_ptr<T[]> ptr;
};

template <typename T>
T
expected_capacity(T size)
{
	return size;
}

template <typename T>
constexpr T
expected_sizeof()
{
	return 32;
}

/* access mwthod for container_representation_t */
template <typename T>
T &
get(container_representation_t<T> &container,
    typename container_t<T>::difference_type idx)
{
	return container.ptr[idx];
}

/* if defined persistent segment table */
#elif defined SEGMENT_TABLE

#include <libpmemobj++/experimental/segment_table.hpp>

template <typename T>
using container_t = pmem::obj::experimental::segment_table<T>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type size;
	typename container_t<T>::size_type capacity;

	static const long unsigned int MAX_SEGMENTS = 64;
	/* Underlying segments */
	pmem::obj::experimental::vector<T> ptr[MAX_SEGMENTS];
};

template <typename T>
T
expected_capacity(T size)
{
	typename container_t<T>::size_type pow =
		static_cast<T>(pmem::detail::Log2(size | 1) + 1);
	return T(1) << pow;
}

template <typename T>
constexpr T
expected_sizeof()
{
	return 2064;
}

/* access mwthod for container_representation_t */
template <typename T>
T &
get(container_representation_t<T> &container,
    typename container_t<T>::size_type idx)
{
	using size_type = typename container_t<T>::size_type;
	size_type segment_idx =
		static_cast<size_type>(pmem::detail::Log2(idx | 1));
	size_type local_idx = idx - (size_type(1) << segment_idx);
	return container.ptr[segment_idx][local_idx];
}

#endif
