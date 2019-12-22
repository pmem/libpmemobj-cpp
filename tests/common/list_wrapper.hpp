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

/*
 * list_wrapper.cpp -- wrapper for sharing tests between list-like data
 * structures
 */

/* if defined persistent vector */
#ifdef VECTOR

#include <libpmemobj++/container/vector.hpp>

template <typename T>
using container_t = pmem::obj::vector<T>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type size;
	typename container_t<T>::size_type capacity;
	pmem::obj::persistent_ptr<T[]> ptr;
};

template <typename T>
T
expected_capacity(T value)
{
	return value;
}

constexpr unsigned int
expected_sizeof()
{
	return 32;
}

/* if defined segment vector with default args */
#elif defined SEGMENT_VECTOR_ARRAY_EXPSIZE

#include <libpmemobj++/container/segment_vector.hpp>

namespace pexp = pmem::obj;
template <typename T>
using container_t =
	pexp::segment_vector<T, pexp::exponential_size_array_policy<>>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type segments_used;

	/* Underlying segments */
	pmem::obj::vector<T> ptr[255];
};

template <typename T>
T
expected_capacity(T value)
{
	if (value == 0)
		return 0;
	typename container_t<T>::size_type pow =
		static_cast<T>(pmem::detail::Log2(value | 1)) + 1;
	return T(1) << pow;
}

constexpr unsigned int
expected_sizeof()
{
	return 2056;
}

/* if defined segment vector with vector storage & exponential size*/
#elif defined SEGMENT_VECTOR_VECTOR_EXPSIZE

#include <libpmemobj++/container/segment_vector.hpp>

namespace pexp = pmem::obj;
template <typename T>
using container_t = pexp::segment_vector<T>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type segments_used;

	/* Underlying segments */
	pmem::obj::vector<pmem::obj::vector<T>> ptr;
};

template <typename T>
T
expected_capacity(T value)
{
	if (value == 0)
		return 0;
	typename container_t<T>::size_type pow =
		static_cast<T>(pmem::detail::Log2(value | 1)) + 1;
	return T(1) << pow;
}

constexpr unsigned int
expected_sizeof()
{
	return 40;
}

/* if defined segment vector with fixed segmentation and vector storage */
#elif defined SEGMENT_VECTOR_VECTOR_FIXEDSIZE

#include <libpmemobj++/container/segment_vector.hpp>

namespace pexp = pmem::obj;
template <typename T>
using container_t =
	pexp::segment_vector<T, pexp::fixed_size_vector_policy<100>>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type segments_used;

	/* Underlying segments */
	pmem::obj::vector<pmem::obj::vector<T>> ptr;
};

template <typename T>
T
expected_capacity(T value)
{
	if (value == 0)
		return 0;
	return (((value - 1) / 100) + 1) * 100;
}

constexpr unsigned int
expected_sizeof()
{
	return 40;
}

/* if defined segment vector with fixed large segments and vector storage */
#elif defined SEGMENT_VECTOR_VECTOR_FIXEDSIZE_EXT

#include <libpmemobj++/container/segment_vector.hpp>

namespace pexp = pmem::obj;
template <typename T>
using container_t =
	pexp::segment_vector<T, pexp::fixed_size_vector_policy<15000>>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type segments_used;

	/* Underlying segments */
	pmem::obj::vector<pmem::obj::vector<T>> ptr;
};

template <typename T>
T
expected_capacity(T value)
{
	if (value == 0)
		return 0;
	return (((value - 1) / 15000) + 1) * 15000;
}

constexpr unsigned int
expected_sizeof()
{
	return 40;
}

#endif
