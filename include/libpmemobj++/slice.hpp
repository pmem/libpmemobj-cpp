/*
 * Copyright 2018, Intel Corporation
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
 * Iterface to access sequence of objects.
 */

#ifndef PMEMOBJ_SLICE_HPP
#define PMEMOBJ_SLICE_HPP

#include <iterator>
#include <type_traits>

namespace pmem
{

namespace obj
{

/**
 * pmem::obj::slice - provides interface to access sequence of objects
 */
template <typename Iterator, typename Reference>
class slice {
public:
	using size_type = std::size_t;
	using iterator = Iterator;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using reference = Reference;

	/**
	 * Constructor taking two iterators which define a range
	 *
	 * @throw std::out_of_range if it_end < it_begin
	 */
	slice(Iterator begin, Iterator end)
	    : it_begin(begin), it_end(end)
	{
		if (it_end < it_begin)
			throw std::out_of_range("pmem::obj::slice");
	}

	/**
	 * Defaulted copy constructor
	 */
	slice(const slice &other) noexcept = default;

	/**
	 * Defaulted assignment operator
	 */
	slice &operator=(const slice &other) noexcept = default;

	/**
	 * Returns iterator to the beginning of the range
	 */
	iterator
	begin() const noexcept
	{
		return it_begin;
	}

	/**
	 * Returns iterator to the end of the range
	 */
	iterator
	end() const noexcept
	{
		return it_end;
	}

	/**
	 * Returns reverse iterator to the end
	 */
	reverse_iterator
	rend() const noexcept
	{
		return reverse_iterator(it_begin);
	}

	/**
	 * Returns reverse iterator to the beginning
	 */
	reverse_iterator
	rbegin() const noexcept
	{
		return reverse_iterator(it_end);
	}

	/**
	 * Element access operator
	 *
	 * @throw std::out_of_range if idx is greater or equal to size
	 */
	reference operator[](size_type idx)
	{
		if (idx >= size())
			throw std::out_of_range("pmem::obj::slice");

		return it_begin[idx];
	}

	size_type
	size() const
	{
		return it_end - it_begin;
	}

private:
	iterator it_begin, it_end;
};

} /* namespace obj */

} /* namespace pmem */

#endif /* PMEMOBJ_SLICE_HPP */
