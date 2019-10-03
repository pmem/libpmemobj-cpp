/*
 * Copyright 2018-2019, Intel Corporation
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
 * Array container with std::array compatible interface.
 */

#ifndef LIBPMEMOBJ_CPP_ARRAY_HPP
#define LIBPMEMOBJ_CPP_ARRAY_HPP

#include <algorithm>
#include <functional>

#include <libpmemobj++/container/detail/contiguous_iterator.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj/base.h>

namespace pmem
{

namespace obj
{

/**
 * pmem::obj::array - persistent container with std::array compatible interface.
 *
 * pmem::obj::array can only be stored on pmem. Creating array on
 * stack will result with "pool_error" exception.
 *
 * All methods which allow write access to specific element will add it to an
 * active transaction.
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
		struct _alignment_struct {
			Y _data[1];
		};

		struct alignas(_alignment_struct) type {
			char _data[sizeof(_alignment_struct)];
		};
	};

	/* Member types */
	using value_type = T;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using reference = value_type &;
	using const_reference = const value_type &;
	using iterator = pmem::detail::basic_contiguous_iterator<T>;
	using const_iterator = const_pointer;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using range_snapshotting_iterator =
		pmem::detail::range_snapshotting_iterator<T>;

	/* Underlying array */
	typename standard_array_traits<T, N>::type _data;

	/**
	 * Defaulted constructor.
	 */
	array() = default;

	/**
	 * Defaulted copy constructor.
	 */
	array(const array &) = default;

	/**
	 * Defaulted move constructor.
	 *
	 * Performs member-wise move but do NOT add moved-from array to the
	 * transaction.
	 */
	array(array &&) = default;

	/**
	 * Copy assignment operator - perform assignment from other
	 * pmem::obj::array.
	 *
	 * This function creates a transaction internally.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 */
	array &
	operator=(const array &other)
	{
		/*
		 * _get_pool should be called before self assignment check to
		 * maintain the same behaviour for all arguments.
		 */
		auto pop = _get_pool();

		if (this == &other)
			return *this;

		transaction::run(pop, [&] {
			detail::conditional_add_to_tx(
				this, 1, POBJ_XADD_ASSUME_INITIALIZED);
			std::copy(other.cbegin(), other.cend(), _get_data());
		});

		return *this;
	}

	/**
	 * Move assignment operator - perform move assignment from other
	 * pmem::obj::array.
	 *
	 * This function creates a transaction internally.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 */
	array &
	operator=(array &&other)
	{
		/*
		 * _get_pool should be called before self assignment check to
		 * maintain the same behaviour for all arguments.
		 */
		auto pop = _get_pool();

		if (this == &other)
			return *this;

		transaction::run(pop, [&] {
			detail::conditional_add_to_tx(
				this, 1, POBJ_XADD_ASSUME_INITIALIZED);
			detail::conditional_add_to_tx(
				&other, 1, POBJ_XADD_ASSUME_INITIALIZED);
			std::move(other._get_data(), other._get_data() + size(),
				  _get_data());
		});

		return *this;
	}

	/**
	 * Access element at specific index and add it to a transaction.
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

		detail::conditional_add_to_tx(_get_data() + n, 1,
					      POBJ_XADD_ASSUME_INITIALIZED);

		return _get_data()[n];
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

		return _get_data()[n];
	}

	/**
	 * Access element at specific index.
	 *
	 * @throw std::out_of_range if index is out of bound.
	 */
	const_reference
	const_at(size_type n) const
	{
		if (n >= N)
			throw std::out_of_range("array::const_at");

		return _get_data()[n];
	}

	/**
	 * Access element at specific index and add it to a transaction.
	 * No bounds checking is performed.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reference operator[](size_type n)
	{
		detail::conditional_add_to_tx(_get_data() + n, 1,
					      POBJ_XADD_ASSUME_INITIALIZED);

		return _get_data()[n];
	}

	/**
	 * Access element at specific index.
	 * No bounds checking is performed.
	 */
	const_reference operator[](size_type n) const
	{
		return _get_data()[n];
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
		detail::conditional_add_to_tx(this, 1,
					      POBJ_XADD_ASSUME_INITIALIZED);
		return _get_data();
	}

	/**
	 * Returns const raw pointer to the underlying data.
	 */
	const T *
	data() const noexcept
	{
		return _get_data();
	}

	/**
	 * Returns const raw pointer to the underlying data.
	 */
	const T *
	cdata() const noexcept
	{
		return _get_data();
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
		return iterator(_get_data());
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
		return iterator(_get_data() + size());
	}

	/**
	 * Returns const iterator to the beginning.
	 */
	const_iterator
	begin() const noexcept
	{
		return const_iterator(_get_data());
	}

	/**
	 * Returns const iterator to the beginning.
	 */
	const_iterator
	cbegin() const noexcept
	{
		return const_iterator(_get_data());
	}

	/**
	 * Returns a const iterator to the end.
	 */
	const_iterator
	end() const noexcept
	{
		return const_iterator(_get_data() + size());
	}

	/**
	 * Returns a const iterator to the end.
	 */
	const_iterator
	cend() const noexcept
	{
		return const_iterator(_get_data() + size());
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
		return reverse_iterator(iterator(_get_data() + size()));
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
		return reverse_iterator(iterator(_get_data()));
	}

	/**
	 * Returns a const reverse iterator to the beginning.
	 */
	const_reverse_iterator
	rbegin() const noexcept
	{
		return const_reverse_iterator(cend());
	}

	/**
	 * Returns a const reverse iterator to the beginning.
	 */
	const_reverse_iterator
	crbegin() const noexcept
	{
		return const_reverse_iterator(cend());
	}

	/**
	 * Returns a const reverse iterator to the end.
	 */
	const_reverse_iterator
	rend() const noexcept
	{
		return const_reverse_iterator(cbegin());
	}

	/**
	 * Returns a const reverse iterator to the beginning.
	 */
	const_reverse_iterator
	crend() const noexcept
	{
		return const_reverse_iterator(cbegin());
	}

	/**
	 * Access the first element and add this element to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reference
	front()
	{
		detail::conditional_add_to_tx(_get_data(), 1,
					      POBJ_XADD_ASSUME_INITIALIZED);
		return _get_data()[0];
	}

	/**
	 * Access the last element and add this element to a transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 */
	reference
	back()
	{
		detail::conditional_add_to_tx(&_get_data()[size() - 1], 1,
					      POBJ_XADD_ASSUME_INITIALIZED);
		return _get_data()[size() - 1];
	}

	/**
	 * Access the first element.
	 */
	const_reference
	front() const
	{
		return _get_data()[0];
	}

	/**
	 * Access the first element.
	 */
	const_reference
	cfront() const
	{
		return _get_data()[0];
	}

	/**
	 * Access the last element.
	 */
	const_reference
	back() const
	{
		return _get_data()[size() - 1];
	}

	/**
	 * Access the last element.
	 */
	const_reference
	cback() const
	{
		return _get_data()[size() - 1];
	}

	/**
	 * Returns slice and snapshots requested range.
	 *
	 * @param[in] start start index of requested range.
	 * @param[in] n number of elements in range.
	 *
	 * @return slice from start to start + n.
	 *
	 * @throw std::out_of_range if any element of the range would be
	 *	outside of the array.
	 */
	slice<pointer>
	range(size_type start, size_type n)
	{
		if (start + n > N)
			throw std::out_of_range("array::range");

		detail::conditional_add_to_tx(_get_data() + start, n,
					      POBJ_XADD_ASSUME_INITIALIZED);

		return {_get_data() + start, _get_data() + start + n};
	}

	/**
	 * Returns slice.
	 *
	 * @param[in] start start index of requested range.
	 * @param[in] n number of elements in range.
	 * @param[in] snapshot_size number of elements which should be
	 *	snapshotted in a bulk while traversing this slice.
	 *	If provided value is larger or equal to n, entire range is
	 *	added to a transaction. If value is equal to 0 no snapshotting
	 *	happens.
	 *
	 * @return slice from start to start + n.
	 *
	 * @throw std::out_of_range if any element of the range would be
	 *	outside of the array.
	 */
	slice<range_snapshotting_iterator>
	range(size_type start, size_type n, size_type snapshot_size)
	{
		if (start + n > N)
			throw std::out_of_range("array::range");

		if (snapshot_size > n)
			snapshot_size = n;

		return {range_snapshotting_iterator(_get_data() + start,
						    _get_data() + start, n,
						    snapshot_size),
			range_snapshotting_iterator(_get_data() + start + n,
						    _get_data() + start, n,
						    snapshot_size)};
	}

	/**
	 * Returns const slice.
	 *
	 * @param[in] start start index of requested range.
	 * @param[in] n number of elements in range.
	 *
	 * @return slice from start to start + n.
	 *
	 * @throw std::out_of_range if any element of the range would be
	 *	outside of the array.
	 */
	slice<const_iterator>
	range(size_type start, size_type n) const
	{
		if (start + n > N)
			throw std::out_of_range("array::range");

		return {const_iterator(_get_data() + start),
			const_iterator(_get_data() + start + n)};
	}

	/**
	 * Returns const slice.
	 *
	 * @param[in] start start index of requested range.
	 * @param[in] n number of elements in range.
	 *
	 * @return slice from start to start + n.
	 *
	 * @throw std::out_of_range if any element of the range would be
	 *	outside of the array.
	 */
	slice<const_iterator>
	crange(size_type start, size_type n) const
	{
		if (start + n > N)
			throw std::out_of_range("array::crange");

		return {const_iterator(_get_data() + start),
			const_iterator(_get_data() + start + n)};
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
	 * Checks whether array is empty.
	 */
	constexpr bool
	empty() const noexcept
	{
		return size() == 0;
	}

	/**
	 * Fills array with specified value inside internal transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 */
	void
	fill(const_reference value)
	{
		auto pop = _get_pool();

		transaction::run(pop, [&] {
			detail::conditional_add_to_tx(
				this, 1, POBJ_XADD_ASSUME_INITIALIZED);
			std::fill(_get_data(), _get_data() + size(), value);
		});
	}

	/**
	 * Swaps content with other array's content inside internal transaction.
	 *
	 * @throw transaction_error when adding the object to the
	 *		transaction failed.
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 */
	template <std::size_t Size = N>
	typename std::enable_if<Size != 0>::type
	swap(array &other)
	{
		/*
		 * _get_pool should be called before self assignment check to
		 * maintain the same behaviour for all arguments.
		 */
		auto pop = _get_pool();

		if (this == &other)
			return;

		transaction::run(pop, [&] {
			detail::conditional_add_to_tx(
				this, 1, POBJ_XADD_ASSUME_INITIALIZED);
			detail::conditional_add_to_tx(
				&other, 1, POBJ_XADD_ASSUME_INITIALIZED);

			std::swap_ranges(_get_data(), _get_data() + size(),
					 other._get_data());
		});
	}

	/**
	 * Swap for zero-sized array.
	 */
	template <std::size_t Size = N>
	typename std::enable_if<Size == 0>::type
	swap(array &other)
	{
		static_assert(!std::is_const<T>::value,
			      "cannot swap zero-sized array of type 'const T'");
	}

private:
	/**
	 * Support for non-zero sized array.
	 */
	template <std::size_t Size = N>
	typename std::enable_if<Size != 0, T *>::type
	_get_data()
	{
		return this->_data;
	}

	/**
	 * Support for non-zero sized array.
	 */
	template <std::size_t Size = N>
	typename std::enable_if<Size != 0, const T *>::type
	_get_data() const
	{
		return this->_data;
	}

	/**
	 * Support for zero sized array.
	 * Return value is a unique address (address of the array itself);
	 */
	template <std::size_t Size = N>
	typename std::enable_if<Size == 0, T *>::type
	_get_data()
	{
		return reinterpret_cast<T *>(&this->_data);
	}

	/**
	 * Support for zero sized array.
	 */
	template <std::size_t Size = N>
	typename std::enable_if<Size == 0, const T *>::type
	_get_data() const
	{
		return reinterpret_cast<const T *>(&this->_data);
	}

	/**
	 * Check whether object is on pmem and return pool_base instance.
	 *
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 */
	pool_base
	_get_pool() const
	{
		auto pop = pmemobj_pool_by_ptr(this);
		if (pop == nullptr)
			throw pmem::pool_error(
				"Object outside of pmemobj pool.");

		return pool_base(pop);
	}
};

/**
 * Non-member equal operator.
 */
template <typename T, std::size_t N>
inline bool
operator==(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

/**
 * Non-member not-equal operator.
 */
template <typename T, std::size_t N>
inline bool
operator!=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs == rhs);
}

/**
 * Non-member less than operator.
 */
template <typename T, std::size_t N>
inline bool
operator<(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return std::lexicographical_compare(lhs.cbegin(), lhs.cend(),
					    rhs.cbegin(), rhs.cend());
}

/**
 * Non-member greater than operator.
 */
template <typename T, std::size_t N>
inline bool
operator>(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return rhs < lhs;
}

/**
 * Non-member greater or equal operator.
 */
template <typename T, std::size_t N>
inline bool
operator>=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs < rhs);
}

/**
 * Non-member less or equal operator.
 */
template <typename T, std::size_t N>
inline bool
operator<=(const array<T, N> &lhs, const array<T, N> &rhs)
{
	return !(lhs > rhs);
}

/**
 * Non-member cbegin.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_iterator
cbegin(const pmem::obj::array<T, N> &a)
{
	return a.cbegin();
}

/**
 * Non-member cend.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_iterator
cend(const pmem::obj::array<T, N> &a)
{
	return a.cend();
}

/**
 * Non-member crbegin.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_reverse_iterator
crbegin(const pmem::obj::array<T, N> &a)
{
	return a.crbegin();
}

/**
 * Non-member crend.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_reverse_iterator
crend(const pmem::obj::array<T, N> &a)
{
	return a.crend();
}

/**
 * Non-member begin.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::iterator
begin(pmem::obj::array<T, N> &a)
{
	return a.begin();
}

/**
 * Non-member begin.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_iterator
begin(const pmem::obj::array<T, N> &a)
{
	return a.begin();
}

/**
 * Non-member end.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::iterator
end(pmem::obj::array<T, N> &a)
{
	return a.end();
}

/**
 * Non-member end.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_iterator
end(const pmem::obj::array<T, N> &a)
{
	return a.end();
}

/**
 * Non-member rbegin.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::reverse_iterator
rbegin(pmem::obj::array<T, N> &a)
{
	return a.rbegin();
}

/**
 * Non-member rbegin.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_reverse_iterator
rbegin(const pmem::obj::array<T, N> &a)
{
	return a.rbegin();
}

/**
 * Non-member rend.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::reverse_iterator
rend(pmem::obj::array<T, N> &a)
{
	return a.rend();
}

/**
 * Non-member rend.
 */
template <typename T, std::size_t N>
typename pmem::obj::array<T, N>::const_reverse_iterator
rend(const pmem::obj::array<T, N> &a)
{
	return a.rend();
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
	return std::move(a.at(I));
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
	return std::move(a.at(I));
}

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_ARRAY_HPP */
