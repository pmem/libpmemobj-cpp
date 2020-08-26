// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/**
 * @file
 * Commonly used SFINAE helpers.
 */

#ifndef LIBPMEMOBJ_CPP_TEMPLATE_HELPERS_HPP
#define LIBPMEMOBJ_CPP_TEMPLATE_HELPERS_HPP

#include <type_traits>

namespace pmem
{

namespace detail
{

template <typename... Ts>
struct make_void {
	typedef void type;
};
template <typename... Ts>
using void_t = typename make_void<Ts...>::type;

/* Generic SFINAE helper for expression checks, based on the idea demonstrated
 * in ISO C++ paper n4502 */
template <typename T, typename, template <typename> class... Checks>
struct supports_impl {
	using type = std::false_type;
};
template <typename T, template <typename> class... Checks>
struct supports_impl<T, void_t<Checks<T>...>, Checks...> {
	using type = std::true_type;
};

template <typename T, template <typename> class... Checks>
using supports = typename supports_impl<T, void, Checks...>::type;

template <typename Compare>
using is_transparent = typename Compare::is_transparent;

template <typename Compare>
using has_is_transparent = detail::supports<Compare, is_transparent>;

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_TEMPLATE_HELPERS_HPP */
