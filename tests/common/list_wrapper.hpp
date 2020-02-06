// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

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
