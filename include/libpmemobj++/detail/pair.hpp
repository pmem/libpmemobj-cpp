/*
 * Copyright 2020, Intel Corporation
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

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_PAIR_HPP */
