// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/detail/enumerable_thread_specific.hpp>

template <typename T>
using container_type = pmem::detail::enumerable_thread_specific<T>;

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	static_assert(
		2128 ==
			sizeof(pmem::obj::shared_mutex) +
				sizeof(pmem::obj::segment_vector<
					char,
					pmem::obj::
						exponential_size_array_policy<>>) +
				sizeof(std::atomic<size_t>),
		"");

	static_assert(sizeof(container_type<int>) == 2128, "");
	static_assert(sizeof(container_type<char>) == 2128, "");
	static_assert(sizeof(container_type<container_type<int>>) == 2128, "");

	static_assert(std::is_standard_layout<container_type<char>>::value, "");
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
