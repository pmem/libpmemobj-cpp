// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

/**
 * @file
 * allocation_flag - defines flags which can be passed to make_persistent
 */

#ifndef LIBPMEMOBJ_CPP_ALLOCATION_FLAG_HPP
#define LIBPMEMOBJ_CPP_ALLOCATION_FLAG_HPP

#include <libpmemobj/base.h>

namespace pmem
{

namespace obj
{

/**
 * Type of flag which can be passed to make_persistent.
 *
 * Allowed flags are:
 * - allocation_flag::class_id(id) - allocate the object from the allocation
 *   class with id equal to id.
 * - allocation_flag::no_flush() - skip flush on commit.
 * - allocation_flag::none() - do not change allocator behaviour.
 *
 * Flags can be combined with each other using operator|()
 */
struct allocation_flag {
	/**
	 * Emplace constructor.
	 */
	explicit allocation_flag(uint64_t val) : value(val)
	{
	}

	/**
	 * Allocate the object from the allocation class with id equal to id.
	 */
	static allocation_flag
	class_id(uint64_t id)
	{
		return allocation_flag(POBJ_CLASS_ID(id));
	}

	/**
	 * Skip flush on commit.
	 */
	static allocation_flag
	no_flush()
	{
		return allocation_flag(POBJ_XALLOC_NO_FLUSH);
	}

	/**
	 * Do not change allocator behaviour.
	 */
	static allocation_flag
	none()
	{
		return allocation_flag(0);
	}

	/**
	 * Check if flag is set.
	 */
	bool
	is_set(const allocation_flag &rhs)
	{
		return (value & rhs.value) != 0;
	}

	allocation_flag
	operator|(const allocation_flag &rhs)
	{
		return allocation_flag(value | rhs.value);
	}

	uint64_t value;
};

/**
 * Type of flag which can be passed to make_persistent_atomic.
 *
 * Allowed flags are:
 * - allocation_flag_atomic::class_id(id) - allocate the object from the
 *   allocation class with id equal to id.
 * - allocation_flag_atomic::none() - do not change allocator behaviour.
 *
 * Flags can be combined with each other using operator|()
 */
struct allocation_flag_atomic {
	/**
	 * Emplace constructor.
	 */
	explicit allocation_flag_atomic(uint64_t val) : value(val)
	{
	}

	/**
	 * Allocate the object from the allocation class with id equal to id.
	 */
	static allocation_flag_atomic
	class_id(uint64_t id)
	{
		return allocation_flag_atomic(POBJ_CLASS_ID(id));
	}

	/**
	 * Do not change allocator behaviour.
	 */
	static allocation_flag_atomic
	none()
	{
		return allocation_flag_atomic(0);
	}

	/**
	 * Check if flag is set.
	 */
	bool
	is_set(const allocation_flag_atomic &rhs)
	{
		return (value & rhs.value) != 0;
	}

	allocation_flag_atomic
	operator|(const allocation_flag_atomic &rhs)
	{
		return allocation_flag_atomic(value | rhs.value);
	}

	uint64_t value;
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_ALLOCATION_FLAG_HPP */
