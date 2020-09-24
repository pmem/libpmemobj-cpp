// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * map_wrapper.cpp -- wrapper for sharing tests between map-like data
 * structures
 */

#ifndef LIBPMEMOBJ_CPP_TESTS_MAP_WRAPPER_HPP
#define LIBPMEMOBJ_CPP_TESTS_MAP_WRAPPER_HPP

/* if concurrent map is defined */
#ifdef LIBPMEMOBJ_CPP_TESTS_CONCURRENT_MAP

#include <libpmemobj++/experimental/concurrent_map.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

template <typename T, typename U, typename Comparator = std::less<T>>
using container_t = nvobjex::concurrent_map<T, U, Comparator>;

template <typename C, typename... Args>
auto
erase(C &m, Args &&... args)
	-> decltype(m.unsafe_erase(std::forward<Args>(args)...))
{
	return m.unsafe_erase(std::forward<Args>(args)...);
}

#define MAP_KEY first
#define MAP_VALUE second
#define TRANSPARENT_COMPARE transparent_less
#define TRANSPARENT_COMPARE_NOT_REFERENCEABLE transparent_less_not_referenceable
#define TRANSPARENT_COMPARE_STRING transparent_less

/* if radix tree is defined */
#elif defined LIBPMEMOBJ_CPP_TESTS_RADIX

#include <libpmemobj++/experimental/radix_tree.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

template <typename T, typename Enable = void>
struct test_bytes_view;

template <typename T>
struct test_bytes_view<
	T, typename std::enable_if<!std::is_signed<T>::value>::type> {
	using type = pmem::detail::bytes_view<T>;
};

struct test_bytes_view_int {
	test_bytes_view_int(const int *v)
	    : v((unsigned)(*v + std::numeric_limits<int>::max() + 1))
	{
	}

	size_t
	size() const
	{
		return sizeof(int);
	}

	char operator[](std::size_t p) const
	{
		return reinterpret_cast<const char *>(&v)[size() - p - 1];
	}

	unsigned v;
};

template <typename T>
struct test_bytes_view<
	T, typename std::enable_if<std::is_signed<T>::value>::type> {
	using type = test_bytes_view_int;
};

/* The third param is comparator but radix does not support that */
template <typename T, typename U,
	  typename BytesView = typename test_bytes_view<T>::type>
using container_t = nvobjex::radix_tree<T, U, BytesView>;

template <typename C, typename... Args>
auto
erase(C &m, Args &&... args) -> decltype(m.erase(std::forward<Args>(args)...))
{
	return m.erase(std::forward<Args>(args)...);
}

#define MAP_KEY key()
#define MAP_VALUE value()
#define TRANSPARENT_COMPARE heterogenous_bytes_view
#define TRANSPARENT_COMPARE_NOT_REFERENCEABLE heterogenous_bytes_view
#define TRANSPARENT_COMPARE_STRING pmem::detail::bytes_view<pmem::obj::string>

#endif

#endif
