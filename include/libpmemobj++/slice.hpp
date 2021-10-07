// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2021, Intel Corporation */

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
 * Provides interface to access sequence of objects.
 *
 * It provides the "view" of any sequence of objects and it simplifies the
 * access to that sequence, with the help of iterators. It's used e.g. in
 * several data structures to deliver `range` methods. As an example please see:
 * pmem::obj::vector::range() .
 *
 * @ingroup data_view
 */
template <typename Iterator>
class slice {
public:
	using size_type = std::size_t;
	using iterator = Iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using reference = typename std::iterator_traits<iterator>::reference;

	/**
	 * Constructor taking two iterators, which define a range.
	 *
	 * @note These iterators have to support: operator[], operator-(), and
	 * operator--()
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
	 * @return iterator to the beginning of the slice's range.
	 */
	iterator
	begin() const noexcept
	{
		return it_begin;
	}

	/**
	 * @return iterator to the end of the slice's range.
	 */
	iterator
	end() const noexcept
	{
		return it_end;
	}

	/**
	 * @return reverse_iterator to the end of the slice's range.
	 */
	reverse_iterator
	rend() const noexcept
	{
		return reverse_iterator(it_begin);
	}

	/**
	 * @return reverse_iterator to the beginning of the slice's range.
	 */
	reverse_iterator
	rbegin() const noexcept
	{
		return reverse_iterator(it_end);
	}

	/**
	 * Access operator for a single element of slice.
	 *
	 * @param idx index of selected element.
	 *
	 * @throw std::out_of_range if idx is greater or equal to size.
	 * @return reference to a selected object.
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
	 * Access operator for a single element of slice. It internally
	 * increments from begin iterator, @param idx positions forward.
	 *
	 * @note No bounds checking is performed, so the iterator may become
	 * invalid.
	 * @return reference to a selected object.
	 */
	reference operator[](size_type idx)
	{
		return it_begin[static_cast<typename std::iterator_traits<
			Iterator>::difference_type>(idx)];
	}

	/**
	 * Returns total number of elements within slice's range.
	 *
	 * @return size_type count of all elements.
	 */
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
