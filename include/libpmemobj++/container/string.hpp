// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2021, Intel Corporation */

/**
 * @file
 * String typedefs for common character types.
 */

#ifndef LIBPMEMOBJ_CPP_STRING_HPP
#define LIBPMEMOBJ_CPP_STRING_HPP

#include <libpmemobj++/container/basic_string.hpp>

namespace pmem
{

namespace obj
{

/**
 * The most typical string usage - the char specialization.
 * @ingroup containers
 */
using string = basic_string<char>;

/**
 * The wide char specialization.
 * @ingroup containers
 */
using wstring = basic_string<wchar_t>;

/**
 * The char16 specialization.
 * @ingroup containers
 */
using u16string = basic_string<char16_t>;

/**
 * The char32 specialization.
 * @ingroup containers
 */
using u32string = basic_string<char32_t>;

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_STRING_HPP */
