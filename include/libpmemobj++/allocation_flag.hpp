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
	allocation_flag() = delete;

	/**
	 * Allocate the object from the allocation class with id equal to id.
	 */
	static allocation_flag
	class_id(uint64_t id)
	{
		return {POBJ_CLASS_ID(id)};
	}

	/**
	 * Skip flush on commit.
	 */
	static allocation_flag
	no_flush()
	{
		return {POBJ_XALLOC_NO_FLUSH};
	}

	/**
	 * Do not change allocator behaviour.
	 */
	static allocation_flag
	none()
	{
		return {0};
	}

	/**
	 * Check if flag is set.
	 */
	bool
	is_set(const allocation_flag &rhs)
	{
		return value & rhs.value;
	}

	allocation_flag
	operator|(const allocation_flag &rhs)
	{
		return {value | rhs.value};
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
	allocation_flag_atomic() = delete;

	/**
	 * Allocate the object from the allocation class with id equal to id.
	 */
	static allocation_flag_atomic
	class_id(uint64_t id)
	{
		return {POBJ_CLASS_ID(id)};
	}

	/**
	 * Do not change allocator behaviour.
	 */
	static allocation_flag_atomic
	none()
	{
		return {0};
	}

	/**
	 * Check if flag is set.
	 */
	bool
	is_set(const allocation_flag_atomic &rhs)
	{
		return value & rhs.value;
	}

	allocation_flag_atomic
	operator|(const allocation_flag_atomic &rhs)
	{
		return {value | rhs.value};
	}

	uint64_t value;
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_ALLOCATION_FLAG_HPP */
