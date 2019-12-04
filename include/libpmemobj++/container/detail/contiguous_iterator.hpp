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
 * Iterators for contiguous persistent containers.
 */

#ifndef LIBPMEMOBJ_CPP_CONTIGUOUS_ITERATOR_HPP
#define LIBPMEMOBJ_CPP_CONTIGUOUS_ITERATOR_HPP

#include <algorithm>
#include <cassert>
#include <functional>

#include <libpmemobj++/detail/common.hpp>

namespace pmem
{

namespace detail
{

/**
 * Base class for iterators which satisfies RandomAccessIterator
 * and operate on contiguous memory.
 */
template <typename Iterator, typename Reference, typename Pointer>
struct contiguous_iterator {
	/**
	 * Constructor taking a pointer.
	 */
	constexpr contiguous_iterator(Pointer begin) : ptr(begin)
	{
	}

	/**
	 * Dereference operator.
	 */
	Reference operator*() const
	{
		return *ptr;
	}

	/**
	 * Arrow operator.
	 */
	Pointer operator->() const
	{
		return ptr;
	}

	/**
	 * Prefix increment operator.
	 */
	Iterator &
	operator++()
	{
		static_cast<Iterator *>(this)->change_by(1);
		return *static_cast<Iterator *>(this);
	}

	/**
	 * Postfix increment operator.
	 */
	Iterator
	operator++(int)
	{
		Iterator tmp(*static_cast<Iterator *>(this));
		static_cast<Iterator *>(this)->change_by(1);
		return tmp;
	}

	/**
	 * Prefix decrement operator.
	 */
	Iterator &
	operator--()
	{
		static_cast<Iterator *>(this)->change_by(-1);
		return *static_cast<Iterator *>(this);
	}

	/**
	 * Postfix decrement operator.
	 */
	Iterator
	operator--(int)
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
	 * Addition operator.
	 */
	Iterator
	operator+(std::ptrdiff_t n) const
	{
		Iterator tmp(*static_cast<const Iterator *>(this));
		tmp += n;
		return tmp;
	}

	/**
	 * Subtraction operator overload for integral type.
	 */
	Iterator
	operator-(std::ptrdiff_t n) const
	{
		Iterator tmp(*static_cast<const Iterator *>(this));
		tmp -= n;
		return tmp;
	}

	/**
	 * Subtraction operator overload Iterator type.
	 */
	friend std::ptrdiff_t
	operator-(const Iterator &lhs, const Iterator &rhs)
	{
		return lhs.ptr - rhs.ptr;
	}

	/**
	 * Element access operator.
	 */
	Reference operator[](std::size_t n)
	{
		return ptr[n];
	}

	Pointer
	get_ptr() const
	{
		return ptr;
	}

protected:
	/**
	 * Function for changing underlying pointer.
	 * This is where static polymorphism is used. Derived classes
	 * can override this method and snapshot data if necessary.
	 */
	void
	change_by(std::ptrdiff_t n)
	{
		ptr += n;
	}

	Pointer ptr;
};

/**
 * Non-const iterator which adds elements to a transaction in a bulk.
 *
 * This is done by dividing underlying array into ranges of specified
 * (snapshot_size) size. If iterator is incremented/decremented/etc.
 * so that it is moved to another range, this new range is added to
 * a transaction.
 *
 * For example, let's assume snapshot_size = 2, N = 6. This gives us:
 * 0  1 | 2  3 | 4  5
 *
 * If iterator is moved from 1 to 3, that means it is now in another
 * range, and that range must be added to a transaction
 * (elements 2 and 3).
 */
template <typename T>
struct range_snapshotting_iterator
    : public contiguous_iterator<range_snapshotting_iterator<T>, T &, T *> {
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using reference = T &;
	using pointer = T *;
	using base_type = contiguous_iterator<range_snapshotting_iterator<T>,
					      reference, pointer>;

	/**
	 * Constructor taking pointer to data, pointer to the beginning
	 * of the array and snapshot_size.
	 */
	range_snapshotting_iterator(pointer ptr = nullptr,
				    pointer data = nullptr,
				    std::size_t size = 0,
				    std::size_t snapshot_size = 1)
	    : base_type(ptr),
	      data(data),
	      size(size),
	      snapshot_size(snapshot_size)
	{
		assert(data <= ptr);

		if (snapshot_size > 0)
			snapshot_range(ptr);
	}

	/**
	 * Conversion operator to const T*.
	 */
	operator const T *() const
	{
		return this->ptr;
	}

	/**
	 * Element access operator.
	 *
	 * Adds element to a transaction.
	 */
	reference operator[](std::size_t n)
	{
		detail::conditional_add_to_tx(&this->ptr[n], 1,
					      POBJ_XADD_ASSUME_INITIALIZED);
		return base_type::operator[](n);
	}

	/**
	 * Non-member swap function.
	 */
	friend void
	swap(range_snapshotting_iterator &lhs, range_snapshotting_iterator &rhs)
	{
		std::swap(lhs.ptr, rhs.ptr);
		std::swap(lhs.data, rhs.data);
		std::swap(lhs.size, rhs.size);
		std::swap(lhs.snapshot_size, rhs.snapshot_size);
	}

	template <typename Iterator, typename Reference, typename Pointer>
	friend struct contiguous_iterator;

protected:
	void
	change_by(std::ptrdiff_t n)
	{
		conditional_snapshot_range(this->ptr, n);
		base_type::change_by(n);
	}

private:
	/*
	 * Conditionally snapshot range of length snapshot_size,
	 * which contain address equal to ptr + diff.
	 */
	void
	conditional_snapshot_range(pointer ptr, difference_type diff)
	{
		if (snapshot_size == 0)
			return;

		auto new_ptr = ptr + diff;

		/* if new pointer is outside of the array */
		if (new_ptr < data || new_ptr >= data + size)
			return;

		/* if new pointer is in the same range */
		if (static_cast<std::size_t>(ptr - data) / snapshot_size ==
		    static_cast<std::size_t>(new_ptr - data) / snapshot_size)
			return;

		snapshot_range(new_ptr);
	}

	void
	snapshot_range(pointer ptr)
	{
		/* align index to snapshot_size */
		auto range_begin =
			ptr - static_cast<uint64_t>(ptr - data) % snapshot_size;
		auto range_size = snapshot_size;

		if (range_begin + range_size > data + size)
			range_size = static_cast<uint64_t>(data + size -
							   range_begin);
#ifndef NDEBUG
		verify_range(range_begin, range_size);
#endif

		detail::conditional_add_to_tx(range_begin, range_size,
					      POBJ_XADD_ASSUME_INITIALIZED);
	}

#ifndef NDEBUG
	void
	verify_range(pointer range_begin, uint64_t range_size)
	{
		auto range_offset = static_cast<uint64_t>(range_begin - data);

		assert(range_begin >= data);
		assert(range_offset % snapshot_size == 0);
		assert((range_offset + range_size) % snapshot_size == 0 ||
		       range_begin + range_size == data + size);
	}
#endif

	pointer data;
	std::size_t size;
	std::size_t snapshot_size;
};

/**
 * Default non-const iterator which adds element to a transaction
 * on every access.
 */
template <typename T>
struct basic_contiguous_iterator
    : public contiguous_iterator<basic_contiguous_iterator<T>, T &, T *> {
	using iterator_category = std::random_access_iterator_tag;
	using value_type = T;
	using difference_type = std::ptrdiff_t;
	using reference = T &;
	using pointer = T *;
	using base_type = contiguous_iterator<basic_contiguous_iterator<T>,
					      reference, pointer>;

	/**
	 * Constructor taking pointer and snapshotting function as
	 * arguments.
	 */
	basic_contiguous_iterator(pointer ptr = nullptr) : base_type(ptr)
	{
	}

	/**
	 * Conversion operator to const T*.
	 */
	operator const T *() const
	{
		return this->ptr;
	}

	/**
	 * Dereference operator which adds dereferenced element to
	 * a transaction.
	 */
	reference operator*() const
	{
		detail::conditional_add_to_tx(this->ptr, 1,
					      POBJ_XADD_ASSUME_INITIALIZED);
		return base_type::operator*();
	}

	/**
	 * Arrow operator which adds underlying element to
	 * a transactions.
	 */
	pointer operator->() const
	{
		detail::conditional_add_to_tx(this->ptr, 1,
					      POBJ_XADD_ASSUME_INITIALIZED);
		return base_type::operator->();
	}

	/**
	 * Element access operator.
	 *
	 * Adds range containing specified element to a transaction.
	 */
	reference operator[](std::size_t n)
	{
		detail::conditional_add_to_tx(&this->ptr[n], 1,
					      POBJ_XADD_ASSUME_INITIALIZED);
		return base_type::operator[](n);
	}

	/**
	 * Non-member swap function.
	 */
	friend void
	swap(basic_contiguous_iterator &lhs, basic_contiguous_iterator &rhs)
	{
		std::swap(lhs.ptr, rhs.ptr);
	}
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_CONTIGUOUS_ITERATOR_HPP */
