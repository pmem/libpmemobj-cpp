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
 * Array container based on std::array
 */

#ifndef PMEMOBJ_ARRAY_HPP
#define PMEMOBJ_ARRAY_HPP

#include <algorithm>
#include <functional>

#include "libpmemobj++/array_iterator.hpp"
#include "libpmemobj++/detail/common.hpp"
#include "libpmemobj++/persistent_ptr.hpp"
#include "libpmemobj++/pext.hpp"
#include "libpmemobj++/slice.hpp"
#include "libpmemobj.h"

#define PMEMOBJ_CONSTEXPR_CXX14

namespace pmem
{

namespace obj
{

/**
 * pmem::obj::array - persistent container based on std::array
 *
 * All methods which allow write access to specific element add this element
 * to a transaction.
 *
 * All methods which return non-const pointer to raw data add entire array
 * to a transaction.
 *
 * When a non-const iterator is returned it adds part of the array
 * to a transaction while traversing.
 */
template <typename T, std::size_t N>
struct array {
	template <typename Y, std::size_t M>
	struct standard_array_traits {
		using type = Y[N];
	};

	/* zero-sized array support */
	template <typename Y>
	struct standard_array_traits<Y, 0> {
		struct type {
			/* clang-format off */
			constexpr operator Y*() const
			{
				return nullptr;
			}
			/* clang-format on */

			constexpr Y &operator[](std::size_t) const
			{
				return *static_cast<Y *>(nullptr);
			}
		};
	};

	/* Member types */
	using value_type = T;
	using pointer = value_type *;
	using const_pointer = const pointer;
	using reference = value_type &;
	using const_reference = const value_type &;
	using iterator = array_iterator<T>;
	using const_iterator = const_array_iterator<T>;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	/* Underlying array */
	typename standard_array_traits<T, N>::type _data;

	/**
	 * Defaulted constructor.
	 */
	constexpr array() = default;

	/**
	 * Defaulted copy constructor.
	 */
	constexpr array(const array<T, N> &) = default;

	/**
	 * Defaulted move constructor to s.
	 */
	constexpr array(array<T, N> &&) = default;

	/**
	 * Copy assignment operator - adds 'this' to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	array &
	operator=(const array &other)
	{
		detail::conditional_add_to_tx(this);

		std::copy(other._data, other._data + other.size(), _data);
		return *this;
	}

	/**
	 * Move assignment operator - adds 'this' to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	array &
	operator=(array &&other)
	{
		detail::conditional_add_to_tx(this);

		std::move(other._data, other._data + other.size(), _data);
		return *this;
	}

	/**
	 * Access element at specific index and add it to a transaction
	 *
	 * @throw std::out_of_range if index is out of bound.
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reference
	at(size_type n)
	{
		if (n >= N)
			throw std::out_of_range("array::at");

		detail::conditional_add_to_tx(_data + n);

		return _data[n];
	}

	/**
	 * Access element at specific index.
	 *
	 * @throw std::out_of_range if index is out of bound.
	 */
	const_reference
	at(size_type n) const
	{
		if (n >= N)
			throw std::out_of_range("array::at");

		return _data[n];
	}

	/**
	 * Access element at specific index and add it to a transaction
	 *
	 * @throw std::out_of_range if index is out of bound.
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reference operator[](size_type n)
	{
		return at(n);
	}

	/**
	 * Access element at specific index.
	 *
	 * @throw std::out_of_range if index is out of bound.
	 */
	const_reference operator[](size_type n) const
	{
		return at(n);
	}

	/**
	 * Returns raw pointer to the underlying data
	 * and adds entire array to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	T *
	data()
	{
		detail::conditional_add_to_tx(this);
		return _data;
	}

	/**
	 * Returns const raw pointer to the underlying data.
	 */
	const T *
	data() const noexcept
	{
		return _data;
	}

	/**
	 * Returns an iterator to the beginning.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	iterator
	begin()
	{
		return make_iterator(_data);
	}

	/**
	 * Returns an iterator to the end.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	iterator
	end()
	{
		return make_iterator(_data + size());
	}

	/**
	 * Returns const iterator to the beginning
	 */
	const_iterator
	begin() const noexcept
	{
		return const_iterator(_data);
	}

	/**
	 * Returns const iterator to the beginning
	 */
	const_iterator
	cbegin() const noexcept
	{
		return const_iterator(_data);
	}

	/**
	 * Returns a const iterator to the end
	 */
	const_iterator
	end() const noexcept
	{
		return const_iterator(_data + size());
	}

	/**
	 * Returns a const iterator to the end
	 */
	const_iterator
	cend() const noexcept
	{
		return const_iterator(_data + size());
	}

	/**
	 * Returns a reverse iterator to the beginning.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reverse_iterator
	rbegin()
	{
		return reverse_iterator(make_iterator(_data + size()));
	}

	/**
	 * Returns a reverse iterator to the end.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reverse_iterator
	rend()
	{
		return reverse_iterator(make_iterator(_data));
	}

	/**
	 * Returns a const reverse iterator to the beginning
	 */
	const_reverse_iterator
	rbegin() const noexcept
	{
		return const_reverse_iterator(cend());
	}

	/**
	 * Returns a const reverse iterator to the beginning
	 */
	const_reverse_iterator
	crbegin() const noexcept
	{
		return const_reverse_iterator(cend());
	}

	/**
	 * Returns a const reverse iterator to the end
	 */
	const_reverse_iterator
	rend() const noexcept
	{
		return const_reverse_iterator(cbegin());
	}

	/**
	 * Returns a const reverse iterator to the beginning
	 */
	const_reverse_iterator
	crend() const noexcept
	{
		return const_reverse_iterator(cbegin());
	}

	/**
	 *  Access the first element and add this element to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reference
	front()
	{
		detail::conditional_add_to_tx(&_data[0]);
		return _data[0];
	}

	/**
	 *  Access the last element and add this element to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reference
	back()
	{
		detail::conditional_add_to_tx(&_data[size() - 1]);
		return _data[size() - 1];
	}

	/**
	 *  Access the first element.
	 */
	const_reference
	front() const
	{
		return _data[0];
	}

	/**
	 *  Access the last element.
	 */
	const_reference
	back() const
	{
		return _data[size() - 1];
	}

	/**
	 * Adds requested range to a transaction and returns slice.
	 *
	 * @param[in] start start index of requested range
	 * @param[in] n number of elements in range
	 * @return slice from start to start + n
	 */
	slice<pointer, reference>
	range(size_type start, size_type n)
	{
		if (start + n >= N)
			throw std::out_of_range("array::slice");

		detail::conditional_add_range_to_tx(_data + start, n);

		return {_data + start, _data + start + n};
	}

	/**
	 * Returns const slice.
	 *
	 * @param[in] start start index of requested range
	 * @param[in] n number of elements in range
	 * @return slice from start to start + n
	 */
	slice<const_iterator, const_reference>
	range(size_type start, size_type n) const
	{
		if (start + n >= N)
			throw std::out_of_range("array::slice");

		return {_data + start, _data + start + n};
	}

	/**
	 * Returns const slice.
	 *
	 * @param[in] start start index of requested range
	 * @param[in] n number of elements in range
	 * @return slice from start to start + n
	 */
	slice<const_iterator, const_reference>
	crange(size_type start, size_type n) const
	{
		if (start + n >= N)
			throw std::out_of_range("array::slice");

		return {_data + start, _data + start + n};
	}

	/**
	 * Returns size of the array.
	 */
	constexpr size_type
	size() const noexcept
	{
		return N;
	}

	/**
	 * Returns the maximum size of the array.
	 */
	constexpr size_type
	max_size() const noexcept
	{
		return N;
	}

	/**
	 * Checks wheter array is empty.
	 */
	constexpr bool
	empty() const noexcept
	{
		return size() != 0;
	}

	/**
	 * Fills array with specified value
	 * and adds entire array to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	void
	fill(const_reference value)
	{
		detail::conditional_add_to_tx(this);
		std::fill(_data, _data + size(), value);
	}

	/**
	 * Swaps content with other array's content.
	 * Adds both arrays to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	void
	swap(array &other)
	{
		detail::conditional_add_to_tx(this);
		detail::conditional_add_to_tx(&other);

		std::swap_ranges(_data, _data + size(), other._data);
	}

private:
	static constexpr bool small_array_snapshotting = N * sizeof(T) < 1024;
	static constexpr std::size_t snapshot_length =
		small_array_snapshotting ? N : 1024 / sizeof(T);

	/*
	 * Function for snapshoting range of size snapshot_length * sizeof(T),
	 * which contain address equal to ptr + diff. First range start at
	 * _data,
	 * second at _data + snapshot_length, etc.
	 *
	 * Snapshotting is done only if ptr + diff points to an element inside
	 * the array and ptr + diff is in different range than ptr.
	 */
	template <bool NonZero = N != 0>
	typename std::enable_if<NonZero>::type
	snapshot_range(pointer ptr, difference_type diff)
	{
		auto new_ptr = ptr + diff;

		if (new_ptr < _data || new_ptr >= _data + N)
			return;

		if ((ptr - _data) / snapshot_length ==
		    (new_ptr - _data) / snapshot_length)
			return;

		auto range_begin =
			new_ptr - (new_ptr - _data) % snapshot_length;

		detail::conditional_add_range_to_tx(
			range_begin, snapshot_length * sizeof(T));
	}

	/*
	 * When snapshot_length < N it is iterator's responsibility to call
	 * snapshotting function
	 */
	template <bool flag = small_array_snapshotting>
	typename std::enable_if<!flag, iterator>::type
	make_iterator(pointer ptr)
	{
		detail::conditional_add_range_to_tx(ptr, snapshot_length *
							    sizeof(T));

		return iterator(ptr, [&](pointer ptr, difference_type diff) {
			snapshot_range(ptr, diff);
		});
	}

	/*
	 * When snapshot_length >= N - entire array is added to a transaction
	 * and iterators do nothing
	 */
	template <bool flag = small_array_snapshotting>
	typename std::enable_if<flag, iterator>::type
	make_iterator(pointer ptr)
	{
		detail::conditional_add_to_tx(this);
		return iterator(ptr);
	}
};

template <typename T, std::size_t N>
inline bool
operator==(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename T, std::size_t N>
inline bool
operator!=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs == rhs);
}

template <typename T, std::size_t N>
inline bool
operator<(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return std::lexicographical_compare(lhs.cbegin(), lhs.cend(),
					    rhs.cbegin(), rhs.cend());
}

template <typename T, std::size_t N>
inline bool
operator>(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return rhs < lhs;
}

template <typename T, std::size_t N>
inline bool
operator>=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs < rhs);
}

template <typename T, std::size_t N>
inline bool
operator<=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs > rhs);
}

} /* namespace obj */

} /* namespace pmem */

namespace std
{

/**
 * Specialization of cbegin, cend, crbegin, crend
 * Std versions of this functions could call non-const versions.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_iterator
cbegin(const pmem::obj::array<T, N> &a)
{
	return a.cbegin();
}

template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_iterator
cend(const pmem::obj::array<T, N> &a)
{
	return a.cend();
}

template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_reverse_iterator
crbegin(const pmem::obj::array<T, N> &a)
{
	return a.crbegin();
}

template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_reverse_iterator
crend(const pmem::obj::array<T, N> &a)
{
	return a.crend();
}

/**
 * Non-member swap function.
 */
template <typename T, size_t N>
inline void
swap(pmem::obj::array<T, N> &lhs, pmem::obj::array<T, N> &rhs)
{
	lhs.swap(rhs);
}

/**
 * Non-member get function.
 */
template <size_t I, typename T, size_t N>
T &
get(pmem::obj::array<T, N> &a)
{
	static_assert(I < N,
		      "Index out of bounds in std::get<> (pmem::obj::array)");
	return a.at(I);
}

/**
 * Non-member get function.
 */
template <size_t I, typename T, size_t N>
T &&
get(pmem::obj::array<T, N> &&a)
{
	static_assert(I < N,
		      "Index out of bounds in std::get<> (pmem::obj::array)");
	return move(a.at(I));
}

/**
 * Non-member get function.
 */
template <size_t I, typename T, size_t N>
const T &
get(const pmem::obj::array<T, N> &a) noexcept
{
	static_assert(I < N,
		      "Index out of bounds in std::get<> (pmem::obj::array)");
	return a.at(I);
}

/**
 * Non-member get function.
 */
template <size_t I, typename T, size_t N>
const T &&
get(const pmem::obj::array<T, N> &&a) noexcept
{
	static_assert(I < N,
		      "Index out of bounds in std::get<> (pmem::obj::array)");
	return move(a.at(I));
}

/**
 *  Tuple interface to class template array.
 */
template <typename T, size_t N>
class tuple_size<pmem::obj::array<T, N>>
	: public std::integral_constant<size_t, N> {
};

/**
 *  Tuple interface to class template array.
 */
template <size_t I, class T, size_t N>
struct tuple_element<I, pmem::obj::array<T, N>> {
	using type = T;
};

} /* namespace std */

#endif /* PMEMOBJ_ARRAY_HPP */
