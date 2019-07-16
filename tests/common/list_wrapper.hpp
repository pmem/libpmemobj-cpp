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
expected_capacity(T value)
{
	return value;
}

template <typename T>
struct expected_sizeof {
	constexpr static int value{32};
};

/* if defined persistent segment table */
#elif defined STATIC_SEGMENT_VECTOR

#include <libpmemobj++/experimental/segment_vector.hpp>

template <typename T>
using container_t = pmem::obj::experimental::segment_vector<T>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type segments_used;

	static const long unsigned int MAX_SEGMENTS =
		pmem::obj::storage_policy<T>::MAX_SEGMENTS;
	/* Underlying segments */
	pmem::obj::experimental::vector<T> ptr[MAX_SEGMENTS];
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

template <typename T>
struct expected_sizeof {
	constexpr static int value{776};
};

template <>
struct expected_sizeof<int> {
	constexpr static int value{1032};
};

template <>
struct expected_sizeof<char> {
	constexpr static int value{1096};
};

#elif defined SEGMENT_VECTOR

#include <libpmemobj++/experimental/segment_vector.hpp>

template <typename T>
using container_t = pmem::obj::experimental::segment_vector<
	T, pmem::obj::vector_segment_policy<T>, pmem::obj::storage_policy<T>>;

template <typename T>
struct container_representation_t {
	typename container_t<T>::size_type segments_used;

	static const long unsigned int MAX_SEGMENTS =
		pmem::obj::storage_policy<T>::MAX_SEGMENTS;
	/* Underlying segments */
	pmem::obj::experimental::vector<pmem::obj::experimental::vector<T>> ptr;
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

template <typename T>
struct expected_sizeof {
	constexpr static int value{40};
};

#endif
