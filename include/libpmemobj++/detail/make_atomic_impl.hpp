// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2019, Intel Corporation */

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
