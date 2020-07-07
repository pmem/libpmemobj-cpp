// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * map_wrapper.cpp -- wrapper for sharing tests between map-like data
 * structures
 */

/* if concurrent map is defined */
#ifdef CONCURRENT_MAP

#include <libpmemobj++/experimental/concurrent_map.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

template <typename T, typename U>
using container_t = nvobjex::concurrent_map<T, U>;

container_t<int, double>::size_type
erase(container_t<int, double> &m, int pos)
{
	return m.unsafe_erase(pos);
}

/* if radix tree is defined */
#elif defined RADIX

#include <libpmemobj++/experimental/radix.hpp >
namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

template <typename T>
using container_t = pexp::radix_tree<T>;

#endif
