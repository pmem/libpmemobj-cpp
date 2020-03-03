// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

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

using string = basic_string<char>;
using wstring = basic_string<wchar_t>;
using u16string = basic_string<char16_t>;
using u32string = basic_string<char32_t>;

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_STRING_HPP */
