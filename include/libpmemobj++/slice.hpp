/*
 * Copyright 2018-2020, Intel Corporation
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

/**
 * @file
 * Interface to access sequence of objects.
 */

#ifndef LIBPMEMOBJ_CPP_SLICE_HPP
#define LIBPMEMOBJ_CPP_SLICE_HPP

#include <iterator>
#include <stdexcept>
#include <type_traits>

namespace pmem
{

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
	 * Constructor taking two RandomAccess iterators which define a range.
	 *
	 * @throw std::out_of_range if it_end < it_begin.
	 */
	slice(Iterator begin, Iterator end) : it_begin(begin), it_end(end)
	{
		static_assert(
			std::is_same<typename std::iterator_traits<
					     iterator>::iterator_category,
				     std::random_access_iterator_tag>::value,
			"Iterator should have RandomAccessIterator tag");

		if (it_end < it_begin)
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

		return it_begin[idx];
	}

	/**
	 * Element access operator.
	 * No bounds checking is performed.
	 */
	reference operator[](size_type idx)
	{
		return it_begin[idx];
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
