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
 * Canonical implementations of standard category iterators.
 */

#ifndef ITERATORS_COMMON_HPP
#define ITERATORS_COMMON_HPP

#include <iterator>

namespace test_common
{

/**
 * Canonical implementation of OutputIterator. Satisfy requirements:
 * - copy-constructible
 * - copy-assignable
 * - destructible
 * - can be incremented
 * - can be dereferenced as an lvalue
 */
template <typename T>
class output_it {
public:
	using iterator_category = std::output_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T *;
	using reference = T &;

	output_it() = delete;

	explicit output_it(T it) : _it(it)
	{
	}

	output_it(const output_it &t) : _it(t._it)
	{
	}

	reference operator*() const
	{
		return *_it;
	}

	output_it &
	operator++()
	{
		++_it;
		return *this;
	}

	output_it
	operator++(int)
	{
		output_it tmp(*this);
		++(*this);
		return tmp;
	}

private:
	T _it;
};

/**
 * Canonical implementation of InputIterator. Satisfy requirements:
 * - copy-constructible
 * - copy-assignable
 * - destructible
 * - can be incremented
 * - can be dereferenced as an rvalue
 * - supports equality/inequality comparisons
 */
template <typename T>
class input_it {
public:
	using iterator_category = std::input_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T *;
	using reference = T &;

	input_it() = delete;

	explicit input_it(T it) : _it(it)
	{
	}

	input_it(const input_it &t) : _it(t._it)
	{
	}

	reference operator*() const
	{
		return *_it;
	}

	const pointer operator->() const
	{
		return _it;
	}

	input_it &
	operator++()
	{
		++_it;
		return *this;
	}

	input_it
	operator++(int)
	{
		input_it tmp(*this);
		++(*this);
		return tmp;
	}

	friend bool
	operator==(const input_it &x, const input_it &y)
	{
		return x._it == y._it;
	}

	friend bool
	operator!=(const input_it &x, const input_it &y)
	{
		return !(x == y);
	}

private:
	T _it;
};

/**
 * Canonical implementation of ForwardIterator. Satisfy requirements:
 * - copy-constructible
 * - copy-assignable
 * - default-constructible
 * - destructible
 * - can be incremented
 * - can be dereferenced as an rvalue
 * - can be dereferenced as an lvalue
 * - supports equality/inequality comparisons
 * - multi-pass: neither dereferencing nor incrementing affects
 * dereferenceability
 */
template <typename T>
class forward_it {
public:
	using iterator_category = std::forward_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T *;
	using reference = T &;

	forward_it() : _it()
	{
	}

	explicit forward_it(T it) : _it(it)
	{
	}

	forward_it(const forward_it &t) : _it(t._it)
	{
	}

	reference operator*() const
	{
		return *_it;
	}

	pointer operator->() const
	{
		return _it;
	}

	forward_it &
	operator++()
	{
		++_it;
		return *this;
	}

	forward_it
	operator++(int)
	{
		forward_it tmp(*this);
		++(*this);
		return tmp;
	}

	friend bool
	operator==(const forward_it &x, const forward_it &y)
	{
		return x._it == y._it;
	}

	friend bool
	operator!=(const forward_it &x, const forward_it &y)
	{
		return !(x == y);
	}

private:
	T _it;
};

/**
 * Canonical implementation of BidirectionalIterator. Satisfy requirements:
 * - copy-constructible
 * - copy-assignable
 * - default-constructible
 * - destructible
 * - can be incremented
 * - can be decremented
 * - can be dereferenced as an rvalue
 * - can be dereferenced as an lvalue
 * - supports equality/inequality comparisons
 * - multi-pass: neither dereferencing nor incrementing affects
 * dereferenceability
 */
template <typename T>
class bidirectional_it {
public:
	using iterator_category = std::bidirectional_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T *;
	using reference = T &;

	bidirectional_it() : _it()
	{
	}

	explicit bidirectional_it(T it) : _it(it)
	{
	}

	bidirectional_it(const bidirectional_it &t) : _it(t._it)
	{
	}

	reference operator*() const
	{
		return *_it;
	}

	pointer operator->() const
	{
		return _it;
	}

	bidirectional_it &
	operator++()
	{
		++_it;
		return *this;
	}

	bidirectional_it
	operator++(int)
	{
		bidirectional_it tmp(*this);
		++(*this);
		return tmp;
	}

	bidirectional_it &
	operator--()
	{
		--_it;
		return *this;
	}

	bidirectional_it
	operator--(int)
	{
		bidirectional_it tmp(*this);
		--(*this);
		return tmp;
	}

	friend bool
	operator==(const bidirectional_it &x, const bidirectional_it &y)
	{
		return x._it == y._it;
	}

	friend bool
	operator!=(const bidirectional_it &x, const bidirectional_it &y)
	{
		return !(x == y);
	}

private:
	T _it;
};

/**
 * Canonical implementation of RandomAccessIterator. Satisfy requirements:
 * - copy-constructible
 * - copy-assignable
 * - default-constructible
 * - destructible
 * - can be incremented
 * - can be decremented
 * - supports arithmetic operators + and -
 * - supports inequality comparisons (<, >, <= and >=) between iterators
 * - supports compound assignment operations += and -=
 * - supports offset dereference operator ([])
 * - can be dereferenced as an rvalue
 * - can be dereferenced as an lvalue
 * - supports equality/inequality comparisons
 * - multi-pass: neither dereferencing nor incrementing affects
 * dereferenceability
 */
template <typename T>
class random_access_it {
public:
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using pointer = T *;
	using reference = T &;

	random_access_it() : _it()
	{
	}

	explicit random_access_it(T it) : _it(it)
	{
	}

	random_access_it(const random_access_it &t) : _it(t._it)
	{
	}

	reference operator*() const
	{
		return *_it;
	}

	pointer operator->() const
	{
		return _it;
	}

	random_access_it &
	operator++()
	{
		++_it;
		return *this;
	}

	random_access_it
	operator++(int)
	{
		random_access_it tmp(*this);
		++(*this);
		return tmp;
	}

	random_access_it &
	operator--()
	{
		--_it;
		return *this;
	}

	random_access_it
	operator--(int)
	{
		random_access_it tmp(*this);
		--(*this);
		return tmp;
	}

	random_access_it &
	operator+=(difference_type n)
	{
		_it += n;
		return *this;
	}

	random_access_it
	operator+(difference_type n) const
	{
		random_access_it tmp(*this);
		tmp += n;
		return tmp;
	}

	friend random_access_it
	operator+(difference_type n, random_access_it x)
	{
		x += n;
		return x;
	}

	random_access_it &
	operator-=(difference_type n)
	{
		return *this += -n;
	}

	random_access_it
	operator-(difference_type n) const
	{
		random_access_it tmp(*this);
		tmp -= n;
		return tmp;
	}

	reference operator[](difference_type n) const
	{
		return _it[n];
	}

	friend bool
	operator==(const random_access_it &x, const random_access_it &y)
	{
		return x._it == y._it;
	}

	friend bool
	operator!=(const random_access_it &x, const random_access_it &y)
	{
		return !(x == y);
	}

	friend bool
	operator<(const random_access_it &x, const random_access_it &y)
	{
		return x._it() < y._it();
	}

	friend bool
	operator<=(const random_access_it &x, const random_access_it &y)
	{
		return !(y < x);
	}

	friend bool
	operator>(const random_access_it &x, const random_access_it &y)
	{
		return y < x;
	}

	friend bool
	operator>=(const random_access_it &x, const random_access_it &y)
	{
		return !(x < y);
	}

private:
	T _it;
};

} /* namespace test_common */

#endif /* ITERATORS_COMMON_HPP */
