// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMOBJ_PAIR_HPP
#define LIBPMEMOBJ_PAIR_HPP

#include <utility>

#include <libpmemobj++/detail/integer_sequence.hpp>

namespace pmem
{

namespace detail
{

template <typename F, typename S>
struct pair {
	constexpr pair() : first(), second()
	{
	}

	template <typename... Args1, typename... Args2>
	pair(std::piecewise_construct_t pc, std::tuple<Args1...> first_args,
	     std::tuple<Args2...> second_args)
	    : pair(pc, first_args, second_args,
		   typename make_index_sequence<Args1...>::type{},
		   typename make_index_sequence<Args2...>::type{})
	{
	}

	constexpr pair(const F &k, const S &v) : first(k), second(v)
	{
	}

	template <typename K, typename V>
	constexpr pair(K &&k, V &&v)
	    : first(std::forward<K>(k)), second(std::forward<V>(v))
	{
	}

	template <typename K, typename V>
	constexpr pair(const std::pair<K, V> &p)
	    : first(p.first), second(p.second)
	{
	}

	template <typename K, typename V>
	constexpr pair(std::pair<K, V> &&p)
	    : first(std::forward<K>(p.first)), second(std::forward<V>(p.second))
	{
	}

	F first;
	S second;

private:
	template <typename... Args1, typename... Args2, size_t... I1,
		  size_t... I2>
	pair(std::piecewise_construct_t, std::tuple<Args1...> &first_args,
	     std::tuple<Args2...> &second_args, index_sequence<I1...>,
	     index_sequence<I2...>)
	    : first(std::forward<Args1>(std::get<I1>(first_args))...),
	      second(std::forward<Args2>(std::get<I2>(second_args))...)
	{
	}
};

template <class T1, class T2>
bool
operator==(const pair<T1, T2> &lhs, const pair<T1, T2> &rhs)
{
	return lhs.first == rhs.first && lhs.second == rhs.second;
}

template <class T1, class T2>
bool
operator!=(const pair<T1, T2> &lhs, const pair<T1, T2> &rhs)
{
	return !(lhs == rhs);
}

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_PAIR_HPP */
