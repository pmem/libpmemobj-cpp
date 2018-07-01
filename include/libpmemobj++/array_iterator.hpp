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
 * Iterators for pmem::obj::array
 */

#ifndef PMEMOBJ_ARRAY_ITERATOR_HPP
#define PMEMOBJ_ARRAY_ITERATOR_HPP

#include <algorithm>
#include <functional>

namespace pmem
{

namespace obj
{

/*
 * Base class for array iterators which satisfies RandomAccessIterator
 */
template <typename Iterator, typename Reference, typename Pointer>
struct base_iterator {
	/**
	* Constructor taking a pointer
	*/
	constexpr base_iterator(Pointer begin) : ptr(begin)
	{
	}

	/**
	* Dereference operator
	*/
	constexpr Reference operator*() const
	{
		return *ptr;
	}

	/**
	* Arrow operator
	*/
	constexpr Pointer operator->() const
	{
		return ptr;
	}

	/**
	* Prefix increment operator
	*/
	Iterator &operator++()
	{
		static_cast<Iterator *>(this)->change_by(1);
		return *static_cast<Iterator *>(this);
	}

	/**
	* Postfix increment operator
	*/
	Iterator operator++(int)
	{
		Iterator tmp(*static_cast<Iterator *>(this));
		static_cast<Iterator *>(this)->change_by(1);
		return tmp;
	}

	/**
	* Prefix decrement operator
	*/
	Iterator &operator--()
	{
		static_cast<Iterator *>(this)->change_by(-1);
		return *static_cast<Iterator *>(this);
	}

	/**
	* Postfix decrement operator
	*/
	Iterator operator--(int)
	{
		Iterator tmp(*static_cast<Iterator *>(this));
		static_cast<Iterator *>(this)->change_by(-1);
		return tmp;
	}

	/**
	 * Addition assignment operator.
	 */
	Iterator &
	operator+=(std::ptrdiff_t n)
	{
		static_cast<Iterator *>(this)->change_by(n);
		return *static_cast<Iterator *>(this);
	}

	/**
	* Subtraction assignment operator.
	*/
	Iterator &
	operator-=(std::ptrdiff_t n)
	{
		static_cast<Iterator *>(this)->change_by(-n);
		return *static_cast<Iterator *>(this);
	}

	/**
	 * Addition operator
	 */
	Iterator
	operator+(std::ptrdiff_t n)
	{
		Iterator tmp(*static_cast<Iterator *>(this));
		tmp += n;
		return tmp;
	}

	/**
	 * Subtraction operator overload for integral type
	 */
	Iterator
	operator-(std::ptrdiff_t n)
	{
		Iterator tmp(*static_cast<Iterator *>(this));
		tmp -= n;
		return tmp;
	}

	/**
	 * Subtraction operator overload Iterator type
	 */
	friend std::ptrdiff_t
	operator-(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr - rhs.ptr;
	}

	/**
	 * Element access operator
	 */
	Reference operator[](std::ptrdiff_t n)
	{
		return ptr[n];
	}

	/**
	 * Non-member equal operator
	 */
	constexpr friend bool
	operator==(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr == rhs.ptr;
	}

	/**
	 * Non-member not equal operator
	 */
	constexpr friend bool
	operator!=(const Iterator &lhs, const Iterator &rhs)
	{
		return !(lhs == rhs);
	}

	/**
     * Non-member less than operator
     */
	constexpr friend bool
	operator<(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr < rhs.ptr;
	}

	/**
	 * Non-member greater than operator
	 */
	constexpr friend bool
	operator>(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr > rhs.ptr;
	}

	/**
	 * Non-member less or equal operator
	 */
	constexpr friend bool
	operator<=(const Iterator &lhs, const Iterator &rhs)
	{
		return !(lhs > rhs);
	}

	/**
	 * Non-member greater or equal operator
	 */
	constexpr friend bool
	operator>=(const Iterator &lhs, const Iterator &rhs)
	{
		return !(lhs < rhs);
	}

protected:
	/*
	 * Function for changing underlying pointer.
	 * Non-const iterator may override it and snapshot data if necessary.
	 */
	void
	change_by(std::ptrdiff_t n)
	{
		ptr += n;
	}

	Pointer ptr;
};

template <typename T>
struct const_array_iterator;

template <typename T>
struct array_iterator : public base_iterator<array_iterator<T>, T &, T *> {
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using reference = T &;
	using pointer = T *;
	using base_type = base_iterator<array_iterator<T>, reference, pointer>;

	/**
	 * Constructor taking pointer and snapshotting function as arguments
	 */
	array_iterator(pointer ptr = nullptr,
		       std::function<void(pointer, difference_type)> snapshot =
			       [](...) {})
	    : base_type(ptr), snapshot(snapshot)
	{
	}

	/**
	 * Element access operator.
	 *
	 * Adds range containing specified element to a transaction.
	 */
	reference operator[](std::ptrdiff_t n)
	{
		snapshot(this->ptr, n);
		return base_type::operator[](n);
	}

	/**
	 * Non-member swap function.
	 */
	friend void
	swap(array_iterator &lhs, array_iterator &rhs)
	{
		std::swap(lhs.ptr, rhs.ptr);
		std::swap(lhs.snapshot, rhs.snapshot);
	}

	/* Needed for base_iterator to access overridden 'change_by' */
	template <typename Iterator, typename Reference, typename Pointer>
	friend struct base_iterator;

protected:
	void
	change_by(difference_type n)
	{
		snapshot(this->ptr, n);
		base_type::change_by(n);
	}

	std::function<void(pointer, difference_type)> snapshot;
};

template <typename T>
struct const_array_iterator
	: public base_iterator<const_array_iterator<T>, const T &, const T *> {
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using reference = const T &;
	using pointer = const T *;
	using base_type =
		base_iterator<const_array_iterator<T>, reference, pointer>;

	/**
	 * Constructor taking pointer as argument
	 */
	constexpr const_array_iterator(pointer ptr = nullptr) : base_type(ptr)
	{
	}

	/**
	 * Conversion operator from non-const iterator
	 */
	constexpr const_array_iterator(const array_iterator<T> &other)
	    : base_type(&(*other))
	{
	}

	/**
	 * Non-member swap function.
	 */
	friend void
	swap(const_array_iterator &lhs, const_array_iterator &rhs)
	{
		std::swap(lhs.ptr, rhs.ptr);
	}
};

} /* namespace obj */

} /* namespace pmem */

#endif /* PMEMOBJ_ARRAY_ITERATOR_HPP */
