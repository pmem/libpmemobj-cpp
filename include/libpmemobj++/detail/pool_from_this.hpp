// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * pool_from_this class
 */

#ifndef LIBPMEMOBJ_CPP_POOL_FROM_THIS_HPP
#define LIBPMEMOBJ_CPP_POOL_FROM_THIS_HPP

#include <libpmemobj/base.h>

namespace pmem
{

namespace obj
{
/**
 * This class is base class for all containers, p<> and persistent_ptr<>.
 * It provides method (get_pool()) to obtain pool_base class.
 */
class pool_from_this {

public:
	pool_base
	get_pool()
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
