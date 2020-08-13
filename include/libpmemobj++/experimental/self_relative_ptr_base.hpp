// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * Base class for self_relative_ptr.
 */

#ifndef LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_HPP
#define LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_HPP

#include <libpmemobj++/detail/self_relative_ptr_base_impl.hpp>

namespace pmem
{

namespace obj
{

namespace experimental
{

/**
 * self_relative_ptr base (non-template) class
 *
 * Implements some of the functionality of the self_relative_ptr class. It
 * defines all applicable conversions from and to a self_relative_ptr_base.
 *
 * It can be used e.g. as a parameter, where self_relative_ptr of any template
 * type is required. It is similar to self_relative_ptr<void> (it can point
 * to whatever type), but it can be used when you want to have pointer to some
 * unspecified self_relative_ptr (with self_relative_ptr<void> it can't be done,
 * because: self_relative_ptr<T>* does not convert to self_relative_ptr<void>*).
 */
using self_relative_ptr_base =
	pmem::detail::self_relative_ptr_base_impl<std::ptrdiff_t>;

} /* namespace obj */

} /* namespace pmem */

} /* namespace experimental */

#endif /* LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_HPP */
