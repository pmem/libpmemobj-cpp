// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * pool_from_this class
 */

#ifndef LIBPMEMOBJ_CPP_POOL_FROM_THIS_HPP
#define LIBPMEMOBJ_CPP_POOL_FROM_THIS_HPP

#include <libpmemobj++/pool.hpp>
#include <libpmemobj/base.h>

namespace pmem
{

namespace obj
{
/**
 * This class is a base class for all containers, p<> and persistent_ptr<>.
 * It provides method get_pool() which returns pool_base object where **this**
 * resides.
 */
class pool_from_this {

protected:
	pool_base
	get_pool() const
	{
		PMEMobjpool *pop = pmemobj_pool_by_ptr(this);

		if (pop == nullptr)
			throw pmem::pool_error("Cannot get pool");

		return pool_base(pop);
	}
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_POOL_FROM_THIS_HPP */
