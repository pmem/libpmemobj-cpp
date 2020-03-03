// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

/**
 * @file
 * A volatile data stored along with pmemobjpool. Stores cleanup function which
 * is called on pool close.
 */

#ifndef LIBPMEMOBJ_CPP_POOL_DATA_HPP
#define LIBPMEMOBJ_CPP_POOL_DATA_HPP

#include <atomic>
#include <functional>

namespace pmem
{

namespace detail
{

struct pool_data {
	pool_data()
	{
		initialized = false;
	}

	/* Set cleanup function if not already set */
	void
	set_cleanup(std::function<void()> cleanup)
	{
		bool expected = false;

		/* this is only to protect from concurrent initializations,
		 * there will be no concurrent reads */
		if (initialized.compare_exchange_strong(
			    expected, true, std::memory_order_release,
			    std::memory_order_relaxed)) {
			this->cleanup = cleanup;
		}
	}

	std::atomic<bool> initialized;
	std::function<void()> cleanup;
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_POOL_DATA_HPP */
