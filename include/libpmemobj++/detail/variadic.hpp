// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

/**
 * @file
 * Helper functionality for handling variadic templates.
 */

#ifndef LIBPMEMOBJ_CPP_VARIADIC_HPP
#define LIBPMEMOBJ_CPP_VARIADIC_HPP

namespace pmem
{

namespace detail
{

/*
 * Checks if T is the same type as first type in Args.
 * General case (for sizeof...(Args) == 0).
 */
template <class T, class... Args>
struct is_first_arg_same {
	static constexpr bool value = false;
};

/*
 * Checks if T is the same type as first type in Args.
 * Specialization (for sizeof...(Args) > 0).
 */
template <class T, class FirstArg, class... Args>
struct is_first_arg_same<T, FirstArg, Args...> {
	static constexpr bool value = std::is_same<T, FirstArg>::value;
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_VARIADIC_HPP */
