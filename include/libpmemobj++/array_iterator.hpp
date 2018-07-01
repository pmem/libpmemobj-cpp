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
	base_iterator(Pointer begin) : ptr(begin)
	{
	}

	Reference operator*() const
	{
		return *ptr;
	}

	Pointer operator->() const
	{
		return ptr;
	}

	Iterator &operator++()
	{
		derived_from_this()->change_by(1);
		return *derived_from_this();
	}

	/* postfix ++ */
	Iterator operator++(int)
	{
		Iterator tmp(*derived_from_this());
		derived_from_this()->change_by(1);
		return tmp;
	}

	Iterator &operator--()
	{
		derived_from_this()->change_by(-1);
		return *derived_from_this();
	}

	/* postfix ++ */
	Iterator operator--(int)
	{
		Iterator tmp(*derived_from_this());
		derived_from_this()->change_by(-1);
		return tmp;
	}

	Iterator &
	operator+=(std::ptrdiff_t n)
	{
		derived_from_this()->change_by(n);
		return *derived_from_this();
	}

	Iterator &
	operator-=(std::ptrdiff_t n)
	{
		derived_from_this()->change_by(-n);
		return *derived_from_this();
	}

	Iterator
	operator+(std::ptrdiff_t n)
	{
		Iterator tmp(*derived_from_this());
		tmp += n;
		return tmp;
	}

	Iterator
	operator-(std::ptrdiff_t n)
	{
		Iterator tmp(*derived_from_this());
		tmp -= n;
		return tmp;
	}

	friend std::ptrdiff_t
	operator-(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr - rhs.ptr;
	}

	Reference operator[](std::ptrdiff_t n)
	{
		return ptr[n];
	}

	friend bool
	operator==(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr == rhs.ptr;
	}

	friend bool
	operator!=(const Iterator &lhs, const Iterator &rhs)
	{
		return !(lhs == rhs);
	}

	friend bool
	operator<(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr < rhs.ptr;
	}

	friend bool
	operator>(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr > rhs.ptr;
	}

	friend bool
	operator<=(const Iterator &lhs, const Iterator &rhs)
	{
		return !(lhs > rhs);
	}

	friend bool
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

private:
	Iterator *
	derived_from_this()
	{
		return static_cast<Iterator *>(this);
	}
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

	array_iterator(pointer ptr = nullptr,
		       std::function<void(pointer, difference_type)> snapshot =
			       [](...) {})
	    : base_type(ptr), snapshot(snapshot)
	{
		snapshot(ptr, 0);
	}

	reference operator[](std::ptrdiff_t n)
	{
		snapshot(this->ptr, n);
		return base_type::operator[](n);
	}

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

	const_array_iterator(pointer ptr = nullptr) : base_type(ptr)
	{
	}

	/**
	 * Conversion operator from non-const iterator
	 */
	const_array_iterator(const array_iterator<T> &other)
	    : base_type(&(*other))
	{
	}

	friend void
	swap(const_array_iterator &lhs, const_array_iterator &rhs)
	{
		std::swap(lhs.ptr, rhs.ptr);
	}
};

} /* namespace obj */

} /* namespace pmem */

#endif /* PMEMOBJ_ARRAY_ITERATOR_HPP */
