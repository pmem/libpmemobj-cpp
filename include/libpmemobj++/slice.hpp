// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/**
 * @file
 * Interface to access sequence of objects.
 */

#ifndef LIBPMEMOBJ_CPP_SLICE_HPP
#define LIBPMEMOBJ_CPP_SLICE_HPP

#include <iterator>
#include <stdexcept>
#include <type_traits>

#include "detail/template_helpers.hpp"

namespace pmem
{

namespace detail
{
template <typename Iterator>
using subtraction =
	decltype(std::declval<Iterator>() - std::declval<Iterator>());
template <typename Iterator>
using has_subtraction = supports<Iterator, subtraction>;

template <typename Iterator>
using pre_decrement = decltype(std::declval<Iterator>().operator--());
template <typename Iterator>
using has_pre_decrement = supports<Iterator, pre_decrement>;

template <typename Iterator>
using indexing = decltype(std::declval<Iterator>().operator[](
	std::declval<
		typename std::iterator_traits<Iterator>::difference_type>()));
template <typename Iterator>
using has_indexing = supports<Iterator, indexing>;
} /* namespace detail */

namespace obj
{

/**
 * pmem::obj::slice - provides interface to access
 * sequence of objects.
 */
template <typename Iterator>
class slice {
public:
	using size_type = std::size_t;
	using iterator = Iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using reference = typename std::iterator_traits<iterator>::reference;

	/**
	 * Constructor taking two iterators (iterators should support:
	 * operator[], operator-(), operator--()) which define a range.
	 *
	 * @throw std::out_of_range if it_end < it_begin.
	 */
	slice(Iterator begin, Iterator end) : it_begin(begin), it_end(end)
	{
		static_assert(
			std::is_pointer<Iterator>::value ||
				(detail::has_indexing<Iterator>::value &&
				 detail::has_pre_decrement<Iterator>::value &&
				 detail::has_subtraction<Iterator>::value),
			"Iterator should support: operator[], operator-(), operator--()");

		if (it_end - it_begin < 0)
			throw std::out_of_range("pmem::obj::slice");
	}

	/**
	 * Defaulted copy constructor.
	 */
	slice(const slice &other) noexcept = default;

	/**
	 * Defaulted assignment operator.
	 */
	slice &operator=(const slice &other) noexcept = default;

	/**
	 * Returns iterator to the beginning of the range.
	 */
	iterator
	begin() const noexcept
	{
		return it_begin;
	}

	/**
	 * Returns iterator to the end of the range.
	 */
	iterator
	end() const noexcept
	{
		return it_end;
	}

	/**
	 * Returns reverse iterator to the end.
	 */
	reverse_iterator
	rend() const noexcept
	{
		return reverse_iterator(it_begin);
	}

	/**
	 * Returns reverse iterator to the beginning.
	 */
	reverse_iterator
	rbegin() const noexcept
	{
		return reverse_iterator(it_end);
	}

	/**
	 * Element access operator.
	 *
	 * @throw std::out_of_range if idx is greater or equal to size.
	 */
	reference
	at(size_type idx)
	{
		if (idx >= size())
			throw std::out_of_range("pmem::obj::slice");

		return it_begin[static_cast<typename std::iterator_traits<
			Iterator>::difference_type>(idx)];
	}

	/**
	 * Element access operator.
	 * No bounds checking is performed.
	 */
	reference operator[](size_type idx)
	{
		return it_begin[static_cast<typename std::iterator_traits<
			Iterator>::difference_type>(idx)];
	}

	size_type
	size() const
	{
		return static_cast<size_type>(it_end - it_begin);
	}

private:
	iterator it_begin, it_end;
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_SLICE_HPP */
