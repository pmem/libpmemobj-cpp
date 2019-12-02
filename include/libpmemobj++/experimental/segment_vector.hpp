/*
 * Copyright 2019, Intel Corporation
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
 * A persistent version of segment vector implementation.
 */

#ifndef LIBPMEMOBJ_SEGMENT_VECTOR_HPP
#define LIBPMEMOBJ_SEGMENT_VECTOR_HPP

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/detail/temp_value.hpp>
#include <libpmemobj++/detail/template_helpers.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/transaction.hpp>

#include <vector>

namespace pmem
{
namespace obj
{
namespace experimental
{

namespace segment_vector_internal
{
/**
 * Iterator for segment_vector
 * Since a constant iterator differs only in the type of references and
 * pointers returned by methods, is_const template parameter is
 * responsible for the differences between constant and non-constant
 * iterators
 */
template <typename Container, bool is_const>
class segment_iterator {
public:
	/* Traits */
	using iterator_category = std::random_access_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using table_type = Container;
	using size_type = typename table_type::size_type;
	using value_type = typename table_type::value_type;
	/* Constant dependent traits */
	using table_ptr =
		typename std::conditional<is_const, const table_type *,
					  table_type *>::type;
	using reference =
		typename std::conditional<is_const,
					  typename table_type::const_reference,
					  typename table_type::reference>::type;
	using pointer =
		typename std::conditional<is_const,
					  typename table_type::const_pointer,
					  typename table_type::pointer>::type;

	/* To implement methods where operands differ in constancy */
	friend class segment_iterator<Container, true>;
	friend class segment_iterator<Container, false>;

private:
	/* Pointer to container for iteration */
	table_ptr table;
	/* Index of element in the container */
	size_type index;

public:
	/* Сonstructors */
	segment_iterator() noexcept;
	explicit segment_iterator(table_ptr tab, size_type idx) noexcept;
	segment_iterator(const segment_iterator &other);

	/* Copy ctor to enable conversion from non-const to const
	 * iterator */
	template <typename U = void,
		  typename = typename std::enable_if<is_const, U>::type>
	segment_iterator(const segment_iterator<Container, false> &other);

	/* In(de)crement methods */
	segment_iterator &operator++();
	segment_iterator operator++(int);
	segment_iterator operator+(difference_type idx) const;
	segment_iterator &operator+=(difference_type idx);
	segment_iterator &operator--();
	segment_iterator operator--(int);
	segment_iterator operator-(difference_type idx) const;
	segment_iterator &operator-=(difference_type idx);
	template <bool C>
	difference_type
	operator+(const segment_iterator<Container, C> &rhs) const;
	template <bool C>
	difference_type
	operator-(const segment_iterator<Container, C> &rhs) const;

	/**
	 * Compare methods
	 * Template parameter is needed to enable this methods work with
	 * non-constant and constant interators
	 */
	template <bool C>
	bool operator==(const segment_iterator<Container, C> &rhs) const;
	template <bool C>
	bool operator!=(const segment_iterator<Container, C> &rhs) const;
	template <bool C>
	bool operator<(const segment_iterator<Container, C> &rhs) const;
	template <bool C>
	bool operator>(const segment_iterator<Container, C> &rhs) const;
	template <bool C>
	bool operator<=(const segment_iterator<Container, C> &rhs) const;
	template <bool C>
	bool operator>=(const segment_iterator<Container, C> &rhs) const;

	/* Access methods */
	reference operator*() const;
	pointer operator->() const;
};

/**
 * Default constructor. Constructs an empty segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>::segment_iterator() noexcept
    : table(nullptr), index()
{
}

/**
 * Segment_itarator constructor. Constructs iterator with given
 * container and index to position in it.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>::segment_iterator(table_ptr tab,
							size_type idx) noexcept
    : table(tab), index(idx)
{
}

/**
 * Segment_itarator copy constructor.
 * Constructs iterator based on other.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>::segment_iterator(
	const segment_iterator &other)
    : table(other.table), index(other.index)
{
}

/** Copy constructor for const iterator from non-const iterator. */
template <typename Container, bool is_const>
template <typename U, typename>
segment_iterator<Container, is_const>::segment_iterator(
	const segment_iterator<Container, false> &other)
    : table(other.table), index(other.index)
{
}

/**
 * Prefix increment.
 *
 * @return incremented segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const> &
segment_iterator<Container, is_const>::operator++()
{
	++index;
	return *this;
}

/**
 * Postfix increment.
 *
 * @return incremented segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>
segment_iterator<Container, is_const>::operator++(int)
{
	auto iterator = *this;
	++*this;
	return iterator;
}

/**
 * Random access incrementing.
 *
 * @return incremented segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>
segment_iterator<Container, is_const>::operator+(difference_type idx) const
{
	return segment_iterator(table, index + static_cast<size_type>(idx));
}

/**
 * Random access incrementing with assignment.
 *
 * @return incremented segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const> &
segment_iterator<Container, is_const>::operator+=(difference_type idx)
{
	index += static_cast<size_type>(idx);
	return *this;
}

/**
 * Prefix decrement.
 *
 * @return decremented segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const> &
segment_iterator<Container, is_const>::operator--()
{
	--index;
	return *this;
}

/**
 * Postfix decrement.
 *
 * @return decremented segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>
segment_iterator<Container, is_const>::operator--(int)
{
	auto iterator = *this;
	--*this;
	return iterator;
}

/**
 * Random access decrementing.
 *
 * @return decremented segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>
segment_iterator<Container, is_const>::operator-(difference_type idx) const
{
	return segment_iterator(table, index - static_cast<size_type>(idx));
}

/**
 * Random access decrementing with assignment.
 *
 * @return decremented segment_iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const> &
segment_iterator<Container, is_const>::operator-=(difference_type idx)
{
	index -= static_cast<size_type>(idx);
	return *this;
}

/**
 * Addition operation.
 *
 * @return sum of this and other segment_iterator.
 */
template <typename Container, bool is_const>
template <bool C>
typename segment_iterator<Container, is_const>::difference_type
segment_iterator<Container, is_const>::
operator+(const segment_iterator<Container, C> &rhs) const
{
	return static_cast<difference_type>(index + rhs.index);
}

/**
 * Substraction operation.
 *
 * @return difference between this and other segment_iterator.
 */
template <typename Container, bool is_const>
template <bool C>
typename segment_iterator<Container, is_const>::difference_type
segment_iterator<Container, is_const>::
operator-(const segment_iterator<Container, C> &rhs) const
{
	return static_cast<difference_type>(index - rhs.index);
}

/**
 * Equality operator.
 *
 * @param[in] rhs segment_iterator<Container, C> to compare with.
 *
 * @return true if rhs is equal to current iterator, false otherwise.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator==(const segment_iterator<Container, C> &rhs) const
{
	return (table == rhs.table) && (index == rhs.index);
}

/**
 * Not equal operator.
 *
 * @param[in] rhs segment_iterator<Container, C> to compare with.
 *
 * @return true if rhs is not equal to current iterator, false
 * otherwise.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator!=(const segment_iterator<Container, C> &rhs) const
{
	return (table != rhs.table) || (index != rhs.index);
}

/**
 * Less operator.
 *
 * @param[in] rhs segment_iterator<Container, C> to compare with.
 *
 * @return true if current index less than rhs index, false otherwise.
 *
 * @throw std::invalid_argument if rhs created on other Container
 * instance.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator<(const segment_iterator<Container, C> &rhs) const
{
	if (table != rhs.table)
		throw std::invalid_argument("segment_iterator::operator<");

	return index < rhs.index;
}

/**
 * Greater operator.
 *
 * @param[in] rhs segment_iterator<Container, C> to compare with.
 *
 * @return true if current index greater than rhs index, false
 * otherwise.
 *
 * @throw std::invalid_argument if rhs created on other Container
 * instance.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator>(const segment_iterator<Container, C> &rhs) const
{
	if (table != rhs.table)
		throw std::invalid_argument("segment_iterator::operator<");

	return index > rhs.index;
}

/**
 * Less or equal operator.
 *
 * @param[in] rhs segment_iterator<Container, C> to compare with.
 *
 * @return true if current index less or equal to rhs index, false
 * otherwise.
 *
 * @throw std::invalid_argument if rhs created on other Container
 * instance.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator<=(const segment_iterator<Container, C> &rhs) const
{
	if (table != rhs.table)
		throw std::invalid_argument("segment_iterator::operator<");

	return index <= rhs.index;
}

/**
 * Greater or equal operator.
 *
 * @param[in] rhs segment_iterator<Container, C> to compare with.
 *
 * @return true if current index greater or equal to rhs index, false
 * otherwise.
 *
 * @throw std::invalid_argument if rhs created on other Container
 * instance.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator>=(const segment_iterator<Container, C> &rhs) const
{
	if (table != rhs.table)
		throw std::invalid_argument("segment_iterator::operator<");

	return index >= rhs.index;
}

/**
 * Indirection (dereference).
 */
template <typename Container, bool is_const>
typename segment_iterator<Container, is_const>::reference
	segment_iterator<Container, is_const>::operator*() const
{
	return table->operator[](index);
}

/**
 * Member access.
 */
template <typename Container, bool is_const>
typename segment_iterator<Container, is_const>::pointer
	segment_iterator<Container, is_const>::operator->() const
{
	return &operator*();
}

template <typename Container>
using resize_method =
	decltype(std::declval<Container>().resize(std::declval<size_t>()));

template <typename Container>
using container_has_resize = detail::supports<Container, resize_method>;

template <typename Container, bool = container_has_resize<Container>::value>
struct segment_vector_resize {
	using segment_vector_type = Container;

	static void
	resize(segment_vector_type &c, size_t n)
	{
		c.resize(n);
	}
};

template <typename Container>
struct segment_vector_resize<Container, false> {
	using segment_vector_type = Container;

	static void
	resize(segment_vector_type &c, size_t n)
	{
	}
};

template <typename SegmentVectorType, size_t SegmentSize>
class fixed_size_policy {
public:
	/* Traits */
	using segment_vector_type = SegmentVectorType;
	using segment_type = typename segment_vector_type::value_type;
	using value_type = typename segment_type::value_type;
	using size_type = std::size_t;

	using segment_vector_resize_type =
		segment_vector_resize<segment_vector_type>;

	static constexpr size_type Size = SegmentSize;

	static void
	resize(segment_vector_type &c, size_type n)
	{
		segment_vector_resize_type::resize(c, n);
	}

	/**
	 * @param[in] index - index of element in segment_vector
	 *
	 * @return index of segment where element should locate
	 */
	static size_type
	get_segment(size_type index)
	{
		return index / Size;
	}
	/**
	 * @param[in] segment_index - index of segment
	 *
	 * @return index of first element in segment
	 */
	static size_type
	segment_top(size_type segment_index)
	{
		return segment_index * Size;
	}
	/**
	 * @param[in] segment_index - index of segment
	 *
	 * @return size of segment
	 */
	static size_type
	segment_size(size_type segment_index)
	{
		return Size;
	}
	/**
	 * @param[in] index - index of element in segment_vector
	 *
	 * @return index in segment where it should locate
	 */
	static size_type
	index_in_segment(size_type index)
	{
		return index % Size;
	}

	/**
	 * @return maximum number of elements we can allocate
	 */
	static size_type
	max_size(const SegmentVectorType &seg_storage)
	{
		return seg_storage.max_size() * SegmentSize;
	}

	/**
	 * @return number of elements in range [0, segment_index]
	 */
	static size_type
	capacity(size_type segment_index)
	{
		return (segment_index + 1) * Size;
	}
};

template <typename SegmentVectorType>
class exponential_size_policy {
public:
	/* Traits */
	using segment_vector_type = SegmentVectorType;
	using segment_type = typename segment_vector_type::value_type;
	using value_type = typename segment_type::value_type;
	using size_type = std::size_t;

	using segment_vector_resize_type =
		segment_vector_resize<segment_vector_type>;

	static void
	resize(segment_vector_type &c, size_type n)
	{
		segment_vector_resize_type::resize(c, n);
	}

	/**
	 * @param[in] index - index of element in segment_vector
	 *
	 * @return index of segment where element should locate
	 */
	static size_type
	get_segment(size_type index)
	{
		return static_cast<size_type>(detail::Log2(index | 1));
	}
	/**
	 * @param[in] segment_index - index of segment
	 *
	 * @return index of first element in segment
	 */
	static size_type
	segment_top(size_type segment_index)
	{
		return (size_type(1) << segment_index) & ~size_type(1);
	}
	/**
	 * @param[in] segment_index - index of segment
	 *
	 * @return size of segment
	 */
	static size_type
	segment_size(size_type segment_index)
	{
		return (segment_index == 0) ? 2 : segment_top(segment_index);
	}
	/**
	 * @param[in] index - index of element in segment_vector
	 *
	 * @return index in segment where it should locate
	 */
	static size_type
	index_in_segment(size_type index)
	{
		return index - segment_top(get_segment(index));
	}

	/**
	 * @return maximum number of elements we can allocate
	 */
	static size_t
	max_size(const SegmentVectorType &)
	{
		return segment_size(get_segment(PMEMOBJ_MAX_ALLOC_SIZE /
						sizeof(value_type)) +
				    1);
	}

	/**
	 * @return number of elements in range [0, segment_index]
	 */
	static size_type
	capacity(size_type segment_index)
	{
		if (segment_index == 0)
			return 2;
		return segment_size(segment_index) * 2;
	}
};

} /* segment_vector_internal namespace */

template <typename SegmentType>
using exponential_size_array_policy =
	segment_vector_internal::exponential_size_policy<
		pmem::obj::array<SegmentType, 64>>;

/**
 * XXX: In case of array of segments with fixed segments size we can allocate as
 * much memory as we want, but for tests to pass we need to limit the array size
 */
template <typename SegmentType, size_t SegmentSize>
using fixed_size_array_policy = segment_vector_internal::fixed_size_policy<
	pmem::obj::array<SegmentType, 255>, SegmentSize>;

template <typename SegmentType, size_t SegmentSize>
using fixed_size_vector_policy = segment_vector_internal::fixed_size_policy<
	pmem::obj::vector<SegmentType>, SegmentSize>;

template <typename SegmentType>
using exponential_size_vector_policy =
	segment_vector_internal::exponential_size_policy<
		pmem::obj::vector<SegmentType>>;

/**
 * Segment table is a data type with a vector-like interface
 * The difference is that it does not do reallocations and iterators are
 * not invalidated when adding new elements
 *
 * @pre Segment must contain such functions as: default constructor, destructor,
 * assign, operator[], free_data, emplace_back, clear, resize, reserve, erase,
 * capacity(), size(). They must have signature the same as in vector. Also must
 * support iterators. Examples in class array_storage_policy.
 *
 * @pre Policy must define segment_vector_type as container for Segment.
 * Must have method resize(segment_vector_type, segment_vector_type::size_type),
 * that reserves space for Segments and default constructs them.
 *
 * @pre segment_vector_type must contain const_at(idx), operator[],
 * operator=(&&), swap(&) methods.
 *
 * @pre Policy must define segmentation_policy, that contains methods to manage
 * Segments: get_segment(idx), segment_top(idx), segment_size(idx),
 * index_in_segment(idx), last_segment(), max_size(), capacity(idx) and field
 * MAX_SEGMENTS. Examples and full methods signature in class
 * exponential_segmentation_policy.
 *
 * Segment template represents segment type.
 * Policy template represents Segments storing type and managing methods.
 */
template <typename T, typename Segment = pmem::obj::vector<T>,
	  typename Policy = exponential_size_array_policy<Segment>>
class segment_vector {
public:
	/* Specific traits*/
	using policy_type = Policy;
	using segment_type = typename policy_type::segment_type;
	using segment_vector_type = typename policy_type::segment_vector_type;
	/* Simple access to methods */
	using policy = policy_type;
	using storage = policy_type;

	/* Traits */
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using iterator =
		segment_vector_internal::segment_iterator<segment_vector,
							  false>;
	using const_iterator =
		segment_vector_internal::segment_iterator<segment_vector, true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	/* Constructors */
	segment_vector();
	segment_vector(size_type count, const value_type &value);
	explicit segment_vector(size_type count);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	segment_vector(InputIt first, InputIt last);
	segment_vector(const segment_vector &other);
	segment_vector(segment_vector &&other);
	segment_vector(std::initializer_list<T> init);
	segment_vector(const std::vector<T> &other);

	/* Assign operators */
	segment_vector &operator=(const segment_vector &other);
	segment_vector &operator=(segment_vector &&other);
	segment_vector &operator=(std::initializer_list<T> ilist);
	segment_vector &operator=(const std::vector<T> &other);

	/* Assign methods */
	void assign(size_type count, const_reference value);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	void assign(InputIt first, InputIt last);
	void assign(std::initializer_list<T> ilist);
	void assign(const segment_vector &other);
	void assign(segment_vector &&other);
	void assign(const std::vector<T> &other);

	/* Destructor */
	~segment_vector();

	/* Element access */
	reference at(size_type n);
	const_reference at(size_type n) const;
	const_reference const_at(size_type n) const;
	reference operator[](size_type n);
	const_reference operator[](size_type n) const;
	reference front();
	const_reference front() const;
	const_reference cfront() const;
	reference back();
	const_reference back() const;
	const_reference cback() const;

	/* Iterators */
	iterator begin();
	const_iterator begin() const noexcept;
	const_iterator cbegin() const noexcept;
	iterator end();
	const_iterator end() const noexcept;
	const_iterator cend() const noexcept;
	reverse_iterator rbegin();
	const_reverse_iterator rbegin() const noexcept;
	const_reverse_iterator crbegin() const noexcept;
	reverse_iterator rend();
	const_reverse_iterator rend() const noexcept;
	const_reverse_iterator crend() const noexcept;

	/* Range */
	slice<iterator> range(size_type start, size_type n);
	slice<const_iterator> range(size_type start, size_type n) const;
	slice<const_iterator> crange(size_type start, size_type n) const;

	/* Capacity */
	constexpr bool empty() const noexcept;
	size_type size() const noexcept;
	constexpr size_type max_size() const noexcept;
	void reserve(size_type capacity_new);
	size_type capacity() const noexcept;
	void shrink_to_fit();

	/* Modifiers */
	void clear();
	void free_data();
	iterator insert(const_iterator pos, const T &value);
	iterator insert(const_iterator pos, T &&value);
	iterator insert(const_iterator pos, size_type count, const T &value);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	iterator insert(const_iterator pos, InputIt first, InputIt last);
	iterator insert(const_iterator pos, std::initializer_list<T> ilist);
	template <class... Args>
	iterator emplace(const_iterator pos, Args &&... args);
	template <class... Args>
	reference emplace_back(Args &&... args);
	iterator erase(const_iterator pos);
	iterator erase(const_iterator first, const_iterator last);
	void push_back(const T &value);
	void push_back(T &&value);
	void pop_back();
	void resize(size_type count);
	void resize(size_type count, const value_type &value);
	void swap(segment_vector &other);

private:
	/* Helper functions */
	void internal_reserve(size_type new_capacity);
	template <typename... Args>
	void construct(size_type idx, size_type count, Args &&... args);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	void construct_range(size_type idx, InputIt first, InputIt last);
	void insert_gap(size_type idx, size_type count);
	void shrink(size_type size_new);
	pool_base get_pool() const noexcept;
	void snapshot_data(size_type idx_first, size_type idx_last);

	/* Data structure specific helper functions */
	reference get(size_type n);
	const_reference get(size_type n) const;
	const_reference cget(size_type n) const;
	bool segment_capacity_validation() const;

	/* Number of segments that currently enabled */
	p<size_type> _segments_used = 0;
	/* Segments storage */
	segment_vector_type _data;
};

/* Non-member swap */
template <typename T, typename Segment, typename Policy>
void swap(segment_vector<T, Segment, Policy> &lhs,
	  segment_vector<T, Segment, Policy> &rhs);

/*
 * Comparison operators between
 * pmem::obj::experimental::segment_vector<T, Segment, Policy> and
 * pmem::obj::experimental::segment_vector<T, Segment, Policy>
 */
template <typename T, typename Segment, typename Policy>
bool operator==(const segment_vector<T, Segment, Policy> &lhs,
		const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator!=(const segment_vector<T, Segment, Policy> &lhs,
		const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator<(const segment_vector<T, Segment, Policy> &lhs,
	       const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator<=(const segment_vector<T, Segment, Policy> &lhs,
		const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator>(const segment_vector<T, Segment, Policy> &lhs,
	       const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator>=(const segment_vector<T, Segment, Policy> &lhs,
		const segment_vector<T, Segment, Policy> &rhs);

/*
 * Comparison operators between
 * pmem::obj::experimental::segment_vector<T, Segment, Policy> and
 * std::vector<T>
 */
template <typename T, typename Segment, typename Policy>
bool operator==(const segment_vector<T, Segment, Policy> &lhs,
		const std::vector<T> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator!=(const segment_vector<T, Segment, Policy> &lhs,
		const std::vector<T> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator<(const segment_vector<T, Segment, Policy> &lhs,
	       const std::vector<T> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator<=(const segment_vector<T, Segment, Policy> &lhs,
		const std::vector<T> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator>(const segment_vector<T, Segment, Policy> &lhs,
	       const std::vector<T> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator>=(const segment_vector<T, Segment, Policy> &lhs,
		const std::vector<T> &rhs);

/*
 * Comparison operators between std::vector<T> and
 * pmem::obj::experimental::segment_vector<T, Segment, Policy>
 */
template <typename T, typename Segment, typename Policy>
bool operator==(const std::vector<T> &lhs,
		const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator!=(const std::vector<T> &lhs,
		const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator<(const std::vector<T> &lhs,
	       const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator<=(const std::vector<T> &lhs,
		const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator>(const std::vector<T> &lhs,
	       const segment_vector<T, Segment, Policy> &rhs);
template <typename T, typename Segment, typename Policy>
bool operator>=(const std::vector<T> &lhs,
		const segment_vector<T, Segment, Policy> &rhs);

/**
 * Default constructor. Constructs an empty container.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if wasn't called in transaction.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy>::segment_vector()
{
}

/**
 * Constructs the container with count copies of elements with value
 * value.
 *
 * @param[in] count number of elements to construct.
 * @param[in] value value of all constructed elements.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == count
 * @post capacity() == nearest power of 2 greater than count
 *
 * @throw rethrows constructor exception.
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if wasn't called in transaction.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy>::segment_vector(size_type count,
						   const value_type &value)
{
	internal_reserve(count);
	construct(0, count, value);
}

/**
 * Constructs the container with count copies of T default constructed
 * values.
 *
 * @param[in] count number of elements to construct.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == count
 * @post capacity() == nearest power of 2 greater than count
 *
 * @throw rethrows constructor exception.
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if wasn't called in transaction.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy>::segment_vector(size_type count)
{
	internal_reserve(count);
	construct(0, count);
}

/**
 * Constructs the container with the contents of the range [first,
 * last). The first and last arguments must satisfy InputIterator
 * requirements. This overload only participates in overload resolution
 * if InputIt satisfies InputIterator, to avoid ambiguity with the
 * overload of count-value constructor.
 *
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == std::distance(first, last)
 * @post capacity() == nearest power of 2 greate than
 * std::distance(first, last)
 *
 * @throw rethrows constructor exception.
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if wasn't called in transaction.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate.
 */
template <typename T, typename Segment, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
segment_vector<T, Segment, Policy>::segment_vector(InputIt first, InputIt last)
{
	internal_reserve(static_cast<size_type>(std::distance(first, last)));
	construct_range(0, first, last);
}

/**
 * Copy constructor. Constructs the container with the copy of the
 * contents of other.
 *
 * @param[in] other reference to the segment_vector to be copied.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw rethrows constructor exception.
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if wasn't called in transaction.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy>::segment_vector(const segment_vector &other)
{
	internal_reserve(other.capacity());
	construct_range(0, other.cbegin(), other.cend());
}

/**
 * Move constructor. Constructs the container with the contents of other
 * using move semantics. After the move, other is guaranteed to be
 * empty().
 *
 * @param[in] other rvalue reference to the segment_vector to be moved
 * from.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 * @post other.capacity() == 0
 * @post other.size() == 0
 *
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy>::segment_vector(segment_vector &&other)
{
	_data = std::move(other._data);
	_segments_used = other._segments_used;
	other._segments_used = 0;
}

/**
 * Constructs the container with the contents of the initializer list
 * init.
 *
 * @param[in] init initializer list with content to be constructed.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == init.size()
 * @post capacity() == size()
 *
 * @throw rethrows constructor exception.
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if wasn't called in transaction.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy>::segment_vector(
	std::initializer_list<T> init)
    : segment_vector(init.begin(), init.end())
{
}

/**
 * Copy constructor. Constructs the container with the copy of the
 * contents of std::vector<T> other.
 *
 * @param[in] other reference to the vector to be copied.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw rethrows constructor exception.
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if wasn't called in transaction.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy>::segment_vector(const std::vector<T> &other)
    : segment_vector(other.cbegin(), other.cend())
{
}

/**
 * Copy assignment operator. Replaces the contents with a copy of the
 * contents of other transactionally.
 *
 * @post size() == other.size()
 * @post capacity() == nearest power of 2 greater than max(other.size(),
 * capacity())
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy> &
segment_vector<T, Segment, Policy>::operator=(const segment_vector &other)
{
	assign(other);
	return *this;
}

/**
 * Move assignment operator. Replaces the contents with those of other
 * using move semantics (i.e. the data in other is moved from other into
 * this container) transactionally. Other is in a valid but empty state
 * afterwards.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy> &
segment_vector<T, Segment, Policy>::operator=(segment_vector &&other)
{
	assign(std::move(other));
	return *this;
}

/**
 * Replaces the contents with those identified by initializer list ilist
 * transactionally.
 *
 * @post size() == std::distance(ilist.begin(), ilist.end())
 * @post capacity() == nearest power of 2 greater than max(capacity(),
 * std::distance(ilist.begin(), ilist.end()))
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy> &
segment_vector<T, Segment, Policy>::operator=(std::initializer_list<T> ilist)
{
	assign(ilist.begin(), ilist.end());
	return *this;
}

/**
 * Copy assignment operator. Replaces the contents with a copy of the
 * contents of std::vector<T> other transactionally.
 *
 * @post size() == other.size()
 * @post capacity() == nearest power of 2 greater than max(other.size(),
 * capacity())
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy> &
segment_vector<T, Segment, Policy>::operator=(const std::vector<T> &other)
{
	assign(other);
	return *this;
}

/**
 * Replaces the contents with count copies of value value
 * transactionally. Only the iterators and references in [0,
 * std::distance(first, last)) remain valid.
 *
 * @param[in] count number of elements to construct.
 * @param[in] value value of all constructed elements.
 *
 * @post size() == count
 * @post capacity() == nearest power of 2 greater than max(capacity(),
 * count)
 *
 * @throw std::length_error if count > max_size().
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::assign(size_type count,
					   const_reference value)
{
	if (count > max_size())
		throw std::length_error("Assignable range exceeds max size.");

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (count > capacity())
			internal_reserve(count);
		else if (count < size())
			shrink(count);

		size_type end = policy::get_segment(count - 1);
		for (size_type i = 0; i < end; ++i)
			_data[i].assign(policy::segment_size(i), value);
		_data[end].assign(count - policy::segment_top(end), value);

		_segments_used = end + 1;
	});
	assert(segment_capacity_validation());
}

/**
 * Replaces the contents with copies of those in the range [first, last)
 * transactionally. This overload participates in overload resolution
 * only if InputIt satisfies InputIterator. Only the iterators and
 * references in [0, std::distance(first, last)) remain valid.
 *
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @post size() == std::distance(first, last)
 * @post capacity() == nearest power of 2 greater than max(capacity(),
 * std::distance(first, last))
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
void
segment_vector<T, Segment, Policy>::assign(InputIt first, InputIt last)
{
	size_type count = static_cast<size_type>(std::distance(first, last));
	if (count > max_size())
		throw std::length_error("Assignable range exceeds max size.");

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (count > capacity())
			internal_reserve(count);
		else if (count < size())
			shrink(count);

		difference_type num;
		size_type end = policy::get_segment(count - 1);
		for (size_type i = 0; i < end; ++i) {
			size_type size = policy::segment_size(i);
			num = static_cast<difference_type>(size);
			_data[i].assign(first, std::next(first, num));
			std::advance(first, num);
		}
		num = static_cast<difference_type>(std::distance(first, last));
		_data[end].assign(first, std::next(first, num));

		_segments_used = end + 1;
	});
	assert(segment_capacity_validation());
}

/**
 * Replaces the contents with the elements from the initializer list
 * ilist transactionally. Only the iterators and references in [0,
 * std::distance(ilist.begin(), ilist.end())) remain valid.
 *
 * @param[in] ilist initializer list with content to be constructed.
 *
 * @post size() == std::distance(ilist.begin(), ilist.end())
 * @post capacity() == nearest power of 2 greater than max(capacity(),
 * std::distance(ilist.begin(), ilist.end()))
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::assign(std::initializer_list<T> ilist)
{
	assign(ilist.begin(), ilist.end());
}

/**
 * Copy assignment method. Replaces the contents with a copy of the
 * contents of other transactionally.
 *
 * @post size() == other.size()
 * @post capacity() == max(other.size(), capacity())
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::assign(const segment_vector &other)
{
	if (this != &other)
		assign(other.cbegin(), other.cend());
}

/**
 * Move assignment method. Replaces the contents with those of other
 * using move semantics (i.e. the data in other is moved from other into
 * this container) transactionally. Other is in a valid but empty state
 * afterwards.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::assign(segment_vector &&other)
{
	if (this == &other)
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		_data = std::move(other._data);
		_segments_used = other._segments_used;
		other._segments_used = 0;
	});
}

/**
 * Copy assignment method. Replaces the contents with a copy of the
 * contents of std::vector<T> other transactionally.
 *
 * @post size() == other.size()
 * @post capacity() == max(other.size(), capacity())
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::assign(const std::vector<T> &other)
{
	assign(other.cbegin(), other.cend());
}

/**
 * Destructor.
 * Note that free_data may throw an transaction_free_error when freeing
 * underlying segments failed. It is recommended to call free_data
 * manually before object destruction.
 *
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segments
 * failed.
 */
template <typename T, typename Segment, typename Policy>
segment_vector<T, Segment, Policy>::~segment_vector()
{
	free_data();
}

/**
 * Access element at specific index with bounds checking and add it to a
 * transaction.
 *
 * @param[in] n index number.
 *
 * @return reference to element number n in underlying segments.
 *
 * @throw std::out_of_range if n is not within the range of the
 * container.
 * @throw pmem::transaction_error when adding the object to the
 * transaction failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::reference
segment_vector<T, Segment, Policy>::at(size_type n)
{
	if (n >= size())
		throw std::out_of_range("segment_vector::at");

	detail::conditional_add_to_tx(&get(n), 1, POBJ_XADD_ASSUME_INITIALIZED);

	return get(n);
}

/**
 * Access element at specific index with bounds checking.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying segments.
 *
 * @throw std::out_of_range if n is not within the range of the
 * container.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
segment_vector<T, Segment, Policy>::at(size_type n) const
{
	if (n >= size())
		throw std::out_of_range("segment_vector::at");
	return get(n);
}

/**
 * Access element at specific index with bounds checking. In
 * contradiction to at(), const_at() will return const_reference not
 * depending on the const-qualification of the object it is called on.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying segments.
 *
 * @throw std::out_of_range if n is not within the range of the
 * container.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
segment_vector<T, Segment, Policy>::const_at(size_type n) const
{
	if (n >= size())
		throw std::out_of_range("segment_vector::const_at");
	return get(n);
}

/**
 * Access element at specific index and add it to a transaction. No
 * bounds checking is performed.
 *
 * @param[in] n index number.
 *
 * @return reference to element number n in underlying segments.
 *
 * @throw pmem::transaction_error when adding the object to the
 * transaction failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::reference
	segment_vector<T, Segment, Policy>::operator[](size_type n)
{
	reference element = get(n);

	detail::conditional_add_to_tx(&element, 1,
				      POBJ_XADD_ASSUME_INITIALIZED);

	return element;
}

/**
 * Access element at specific index. No bounds checking is performed.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying segments.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
	segment_vector<T, Segment, Policy>::operator[](size_type n) const
{
	return get(n);
}

/**
 * Access the first element and add this element to a transaction.
 *
 * @return reference to first element in underlying segments.
 *
 * @throw pmem::transaction_error when adding the object to the
 * transaction failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::reference
segment_vector<T, Segment, Policy>::front()
{
	detail::conditional_add_to_tx(&_data[0][0], 1,
				      POBJ_XADD_ASSUME_INITIALIZED);

	return _data[0][0];
}

/**
 * Access the first element.
 *
 * @return const_reference to first element in underlying segments.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
segment_vector<T, Segment, Policy>::front() const
{
	return _data[0][0];
}

/**
 * Access the first element. In contradiction to front(), cfront() will
 * return const_reference not depending on the const-qualification of
 * the object it is called on.
 *
 * @return reference to first element in underlying segments.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
segment_vector<T, Segment, Policy>::cfront() const
{
	return _data[0][0];
}

/**
 * Access the last element and add this element to a transaction.
 *
 * @return reference to the last element in underlying segments.
 *
 * @throw pmem::transaction_error when adding the object to the
 * transaction failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::reference
segment_vector<T, Segment, Policy>::back()
{
	reference element = get(size() - 1);

	detail::conditional_add_to_tx(&element, 1,
				      POBJ_XADD_ASSUME_INITIALIZED);

	return element;
}

/**
 * Access the last element.
 *
 * @return const_reference to the last element in underlying segments.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
segment_vector<T, Segment, Policy>::back() const
{
	return get(size() - 1);
}

/**
 * Access the last element. In contradiction to back(), cback() will
 * return const_reference not depending on the const-qualification of
 * the object it is called on.
 *
 * @return const_reference to the last element in underlying segments.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
segment_vector<T, Segment, Policy>::cback() const
{
	return get(size() - 1);
}

/**
 * Returns an iterator to the beginning.
 *
 * @return iterator pointing to the first element in the segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::begin()
{
	return iterator(this, 0);
}

/**
 * Returns const iterator to the beginning.
 *
 * @return const_iterator pointing to the first element in the
 * segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_iterator
segment_vector<T, Segment, Policy>::begin() const noexcept
{
	return const_iterator(this, 0);
}

/**
 * Returns const iterator to the beginning. In contradiction to begin(),
 * cbegin() will return const_iterator not depending on the
 * const-qualification of the object it is called on.
 *
 * @return const_iterator pointing to the first element in the
 * segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_iterator
segment_vector<T, Segment, Policy>::cbegin() const noexcept
{
	return const_iterator(this, 0);
}

/**
 * Returns an iterator to past the end.
 *
 * @return iterator referring to the past-the-end element in the
 * segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::end()
{
	return iterator(this, size());
}

/**
 * Returns a const iterator to past the end.
 *
 * @return const_iterator referring to the past-the-end element in the
 * segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_iterator
segment_vector<T, Segment, Policy>::end() const noexcept
{
	return const_iterator(this, size());
}

/**
 * Returns a const iterator to the end. In contradiction to end(),
 * cend() will return const_iterator not depending on the
 * const-qualification of the object it is called on.
 *
 * @return const_iterator referring to the past-the-end element in the
 * segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_iterator
segment_vector<T, Segment, Policy>::cend() const noexcept
{
	return const_iterator(this, size());
}

/**
 * Returns a reverse iterator to the beginning.
 *
 * @return reverse_iterator pointing to the last element in the
 * segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::reverse_iterator
segment_vector<T, Segment, Policy>::rbegin()
{
	return reverse_iterator(end());
}

/**
 * Returns a const reverse iterator to the beginning.
 *
 * @return const_reverse_iterator pointing to the last element in the
 * segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reverse_iterator
segment_vector<T, Segment, Policy>::rbegin() const noexcept
{
	return const_reverse_iterator(end());
}

/**
 * Returns a const reverse iterator to the beginning. In contradiction
 * to rbegin(), crbegin() will return const_reverse_iterator not
 * depending on the const-qualification of the object it is called on.
 *
 * @return const_reverse_iterator pointing to the last element in the
 * segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reverse_iterator
segment_vector<T, Segment, Policy>::crbegin() const noexcept
{
	return rbegin();
}

/**
 * Returns a reverse iterator to the end.
 *
 * @return reverse_iterator pointing to the theoretical element
 * preceding the first element in the segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::reverse_iterator
segment_vector<T, Segment, Policy>::rend()
{
	return reverse_iterator(begin());
}

/**
 * Returns a const reverse iterator to the end.
 *
 * @return const_reverse_iterator pointing to the theoretical element
 * preceding the first element in the segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reverse_iterator
segment_vector<T, Segment, Policy>::rend() const noexcept
{
	return const_reverse_iterator(begin());
}

/**
 * Returns a const reverse iterator to the beginning. In contradiction
 * to rend(), crend() will return const_reverse_iterator not depending
 * on the const-qualification of the object it is called on.
 *
 * @return const_reverse_iterator pointing to the theoretical element
 * preceding the first element in the segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reverse_iterator
segment_vector<T, Segment, Policy>::crend() const noexcept
{
	return rend();
}

/**
 * Returns slice and snapshots requested range. This method is not
 * specified by STL standards.
 *
 * @param[in] start start index of requested range.
 * @param[in] n number of elements in range.
 *
 * @return slice from start to start + n.
 *
 * @throw std::out_of_range if any element of the range would be outside
 * of the segment_vector.
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename T, typename Segment, typename Policy>
slice<typename segment_vector<T, Segment, Policy>::iterator>
segment_vector<T, Segment, Policy>::range(size_type start, size_type n)
{
	if (start + n > size())
		throw std::out_of_range("segment_vector::range");

	snapshot_data(start, start + n);

	return {iterator(this, start), iterator(this, start + n)};
}

/**
 * Returns const slice. This method is not specified by STL standards.
 *
 * @param[in] start start index of requested range.
 * @param[in] n number of elements in range.
 *
 * @return slice from start to start + n.
 *
 * @throw std::out_of_range if any element of the range would be outside
 * of the segment_vector.
 */
template <typename T, typename Segment, typename Policy>
slice<typename segment_vector<T, Segment, Policy>::const_iterator>
segment_vector<T, Segment, Policy>::range(size_type start, size_type n) const
{
	if (start + n > size())
		throw std::out_of_range("segment_vector::range");

	return {const_iterator(this, start), const_iterator(this, start + n)};
}

/**
 * Returns const slice. This method is not specified by STL standards.
 *
 * @param[in] start start index of requested range.
 * @param[in] n number of elements in range.
 *
 * @return slice from start to start + n.
 *
 * @throw std::out_of_range if any element of the range would be outside
 * of the segment_vector.
 */
template <typename T, typename Segment, typename Policy>
slice<typename segment_vector<T, Segment, Policy>::const_iterator>
segment_vector<T, Segment, Policy>::crange(size_type start, size_type n) const
{
	if (start + n > size())
		throw std::out_of_range("segment_vector::range");

	return {const_iterator(this, start), const_iterator(this, start + n)};
}

/**
 * Checks whether the container is empty.
 *
 * @return true if container is empty, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
constexpr bool
segment_vector<T, Segment, Policy>::empty() const noexcept
{
	return size() == 0;
}

/**
 * @return number of elements.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::size_type
segment_vector<T, Segment, Policy>::size() const noexcept
{
	size_type result = 0;
	for (size_type i = 0; i < _segments_used; ++i)
		result += _data.const_at(i).size();
	return result;
}

/**
 * @return maximum number of elements the container is able to hold due
 * to PMDK limitations.
 */
template <typename T, typename Segment, typename Policy>
constexpr typename segment_vector<T, Segment, Policy>::size_type
segment_vector<T, Segment, Policy>::max_size() const noexcept
{
	return policy::max_size(_data);
}

/**
 * Increases the capacity of the segment_vector to capacity_new
 * transactionally. If segment where should be capacity_new is greater
 * than the current capacity's segment, new segments allocated,
 * otherwise the method does nothing. Past-the-end iterator invalidated
 * if allocation occurs.
 *
 * @param[in] capacity_new new capacity.
 *
 * @post capacity() == max(capacity(), capacity_new)
 *
 * @throw pmem::length_error when capacity_new larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::reserve(size_type capacity_new)
{
	if (capacity_new <= capacity())
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] { internal_reserve(capacity_new); });
}

/**
 * @return number of elements that can be held in currently allocated
 * storage.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::size_type
segment_vector<T, Segment, Policy>::capacity() const noexcept
{
	if (_segments_used == 0)
		return 0;
	return policy::capacity(_segments_used - 1);
}

/**
 * Requests transactional removal of unused capacity. New capacity will
 * be set to nearest power of 2 greater than current size().
 *
 * @post capacity() == nearest power of 2 bigger than size()
 *
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::shrink_to_fit()
{
	size_type new_last = policy::get_segment(size() - 1);
	if (_segments_used - 1 == new_last)
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		for (size_type i = new_last + 1; i < _segments_used; ++i)
			_data[i].free_data();
		_segments_used = new_last + 1;
		storage::resize(_data, _segments_used);
	});
}

/**
 * Clears the content of a segment_vector transactionally.
 *
 * @post size() == 0
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::clear()
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] { shrink(0); });
	assert(segment_capacity_validation());
}

/**
 * Clears the content of a segment_vector and frees all allocated
 * persistent memory for data transactionally.
 *
 * @post size() == 0
 * @post capacity() == 0
 *
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segments
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::free_data()
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		for (size_type i = 0; i < _segments_used; ++i)
			_data[i].free_data();
		_segments_used = 0;
	});
}

/**
 * Inserts value before pos in the container transactionally. Causes
 * allocation if the new size() is greater than the old capacity(). Only
 * the iterators and references before the insertion point remain valid.
 *
 * @param[in] pos iterator before which the content will be inserted.
 * pos may be the end() iterator.
 * @param[in] value element value to be inserted.
 *
 * @return Iterator pointing to the inserted value.
 *
 * @pre value_type must meet the requirements of CopyAssignable and
 * CopyInsertable.
 *
 * @post capacity() == nearest power of 2 greater than size() + 1, or
 * remains the same if there is enough space.
 *
 * @throw rethrows constructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::insert(const_iterator pos, const T &value)
{
	return insert(pos, 1, value);
}

/**
 * Moves value before pos in the container transactionally. Causes
 * allocation if the new size() is greater than the old capacity(). Only
 * the iterators and references before the insertion pointremain valid.
 *
 * @param[in] pos iterator before which the content will be inserted.
 * pos may be the end() iterator.
 * @param[in] value element value to be inserted.
 *
 * @return Iterator pointing to the inserted value.
 *
 * @pre value_type must meet the requirements of MoveAssignable and
 * MoveInsertable.
 *
 * @post capacity() == nearest power of 2 greater than size() + 1, or
 * remains the same if there is enough space.
 *
 * @throw rethrows constructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::insert(const_iterator pos, T &&value)
{
	size_type idx = static_cast<size_type>(pos - cbegin());

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		insert_gap(idx, 1);
		get(idx) = std::move(value);
	});

	return iterator(this, idx);
}

/**
 * Inserts count copies of the value before pos in the container
 * transactionally. Causes allocation if the new size() is greater than
 * the old capacity(). Only the iterators and references before the
 * insertion point remain valid.
 *
 * @param[in] pos iterator before which the content will be inserted.
 * pos may be the end() iterator.
 * @param[in] count number of copies to be inserted.
 * @param[in] value element value to be inserted.
 *
 * @return Iterator pointing to the first element inserted, or pos if
 * count == 0.
 *
 * @pre value_type must meet the requirements of CopyAssignable and
 * CopyInsertable.
 *
 * @post capacity() == nearest power of 2 greater than size() + count,
 * or remains the same if there is enough space.
 *
 * @throw rethrows constructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::insert(const_iterator pos, size_type count,
					   const T &value)
{
	size_type idx = static_cast<size_type>(pos - cbegin());

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		insert_gap(idx, count);
		for (size_type i = idx; i < idx + count; ++i)
			get(i) = std::move(value);
	});

	return iterator(this, idx);
}

/**
 * Inserts elements from range [first, last) before pos in the container
 * transactionally. Causes allocation if the new size() is greater than
 * the old capacity(). Only the iterators and references before the
 * insertion point remain valid. This overload participates in overload
 * resolution only if InputIt qualifies as InputIterator, to avoid
 * ambiguity with the pos-count-value overload. The behavior is
 * undefined if first and last are iterators into *this.
 *
 * @param[in] pos iterator before which the content will be inserted.
 * pos may be the end() iterator.
 * @param[in] first begin of the range of elements to insert, can't be
 * iterator into container for which insert is called.
 * @param[in] last end of the range of elements to insert, can't be
 * iterator into container for which insert is called.
 *
 * @return Iterator pointing to the first element inserted or pos if
 * first == last.
 *
 * @pre value_type must meet the requirements of EmplaceConstructible,
 * Swappable, CopyAssignable, CopyConstructible and CopyInsertable.
 * @pre InputIt must satisfies requirements of InputIterator.
 *
 * @post capacity() == nearest power of 2 greater than size() +
 * std::distance(first, last), or remains the same if there is
 * enough space.
 *
 * @throw rethrows constructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::insert(const_iterator pos, InputIt first,
					   InputIt last)
{
	size_type idx = static_cast<size_type>(pos - cbegin());
	size_type gap_size = static_cast<size_type>(std::distance(first, last));

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		insert_gap(idx, gap_size);
		for (size_type i = idx; i < idx + gap_size; ++i, ++first)
			get(i) = *first;
	});

	return iterator(this, idx);
}

/**
 * Inserts elements from initializer list ilist before pos in the
 * container transactionally. Causes allocation if the new size() is
 * greater than the old capacity(). Only the iterators and references
 * before the insertion point remain valid.
 *
 * @param[in] pos iterator before which the content will be inserted.
 * pos may be the end() iterator.
 * @param[in] ilist initializer list to insert the values from.
 *
 * @return Iterator pointing to the first element inserted, or pos if
 * ilist is empty.
 *
 * @pre value_type must meet the requirements of EmplaceConstructible,
 * Swappable, CopyAssignable, CopyConstructible and CopyInsertable.
 *
 * @post capacity() == nearest power of 2 greater than size() +
 * std::distance(ilist.begin(), ilist.end()), or remains the same if
 * there is enough space.
 *
 * @throw rethrows constructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::insert(const_iterator pos,
					   std::initializer_list<T> ilist)
{
	return insert(pos, ilist.begin(), ilist.end());
}

/**
 * Inserts a new element into the container directly before pos. The
 * element is constructed in-place. The arguments args... are forwarded
 * to the constructor as std::forward<Args>(args).... Note that standard
 * allows args to be a self reference and internal emplace
 * implementation handles this case by creating temporary element_type
 * object. This object is being stored either on stack or on pmem, see
 * pmem::detail::temp_value for details.
 *
 * @param[in] pos iterator before which the new element will be
 * constructed.
 * @param[in] args arguments to forward to the constructor of the
 * element.
 *
 * @return Iterator pointing to the emplaced element.
 *
 * @pre value_type must meet the requirements of MoveAssignable,
 * MoveInsertable and EmplaceConstructible.
 *
 * @post capacity() == nearest power of 2 greater than size() + 1, or
 * remains the same if there is enough space.
 *
 * @throw rethrows constructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
template <class... Args>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::emplace(const_iterator pos, Args &&... args)
{
	size_type idx = static_cast<size_type>(pos - cbegin());

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		detail::temp_value<value_type,
				   noexcept(T(std::forward<Args>(args)...))>
		tmp(std::forward<Args>(args)...);
		insert_gap(idx, 1);
		get(idx) = std::move(tmp.get());
	});

	return iterator(this, idx);
}

/**
 * Appends a new element to the end of the container transactionally.
 * The element is constructed in-place. The arguments args... are
 * forwarded to the constructor as std::forward<Args>(args)....
 * Past-the-end iterator is invalidated.
 *
 * @param[in] args arguments to forward to the constructor of the
 * element.
 *
 * @return Iterator pointing to the emplaced element.
 *
 * @pre value_type must meet the requirements of MoveInsertable and
 * EmplaceConstructible.
 * @pre size() must be less than max_size()
 *
 * @post capacity() == nearest power of 2 greater than size() + 1, or
 * remains the same if there is enough space.
 *
 * @throw rethrows constructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
template <class... Args>
typename segment_vector<T, Segment, Policy>::reference
segment_vector<T, Segment, Policy>::emplace_back(Args &&... args)
{
	assert(size() < max_size());

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (size() == capacity())
			internal_reserve(capacity() + 1);

		size_type segment = policy::get_segment(size());
		_data[segment].emplace_back(std::forward<Args>(args)...);
	});

	return back();
}

/**
 * Removes the element at pos. Invalidates iterators and references at
 * or after the point of the erase, including the end() iterator. The
 * iterator pos must be valid and dereferenceable. Thus the end()
 * iterator (which is valid, but is not dereferenceable) cannot be used
 * as a value for pos.
 *
 * @param[in] pos iterator to the element to be removed.
 *
 * @return Iterator following the last removed element. If the iterator
 * pos refers to the last element, the end() iterator is returned.
 *
 * @pre value_type must meet the requirements of MoveAssignable.
 *
 * @post size() = size() - 1.
 *
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::erase(const_iterator pos)
{
	return erase(pos, pos + 1);
}

/**
 * Removes the elements in the range [first, last). Invalidates
 * iterators and references at or after the point of the erase,
 * including the end() iterator. The iterator pos must be valid and
 * dereferenceable. Thus the end() iterator (which is valid, but is not
 * dereferenceable) cannot be used as a value for pos.
 *
 * @param[in] first beginning of the range of elements to be removed.
 * @param[in] last end of range of elements to be removed.
 *
 * @return Iterator following the last removed element. If the iterator
 * pos refers to the last element, the end() iterator is returned. If
 * first and last refer to the same element, iterator to this element is
 * returned.
 *
 * @pre value_type must meet the requirements of MoveAssignable.
 *
 * @post size() = size() - std::distance(first, last).
 *
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::iterator
segment_vector<T, Segment, Policy>::erase(const_iterator first,
					  const_iterator last)
{
	size_type count = static_cast<size_type>(std::distance(first, last));
	size_type idx = static_cast<size_type>(first - cbegin());

	if (count == 0)
		return iterator(this, idx);

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		size_type _size = size();

		snapshot_data(idx, _size);

		/* Moving after-range elements to the place of deleted
		 */
		iterator dest = iterator(this, idx);
		iterator begin = iterator(this, idx + count);
		iterator end = iterator(this, _size);
		std::move(begin, end, dest);

		/* Clearing the range where the elements were moved from
		 */
		size_type middle = policy::get_segment(_size - count);
		size_type last = policy::get_segment(_size - 1);
		size_type middle_size = policy::index_in_segment(_size - count);
		for (size_type s = last; s > middle; --s)
			_data[s].clear();
		_data[middle].resize(middle_size);

		_segments_used = middle + 1;
	});

	assert(segment_capacity_validation());

	return iterator(this, idx);
}

/**
 * Appends the given element value to the end of the container
 * transactionally. The new element is initialized as a copy of value.
 *
 * @param[in] value the value of the element to be appended.
 *
 * @pre value_type must meet the requirements of CopyInsertable.
 *
 * @post size() == size() + 1.
 * @post capacity() == nearest power of 2 greater than size() + 1, or
 * remains the same if there is enough space.
 *
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw rethrows constructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::push_back(const T &value)
{
	emplace_back(value);
}

/**
 * Appends the given element value to the end of the container
 * transactionally. value is moved into the new element.
 *
 * @param[in] value the value of the element to be appended.
 *
 * @pre value_type must meet the requirements of MoveInsertable.
 *
 * @post size() == size() + 1.
 * @post capacity() == nearest power of 2 greater than size() + 1, or
 * remains the same if there is enough space.
 *
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw rethrows constructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::push_back(T &&value)
{
	emplace_back(std::move(value));
}

/**
 * Removes the last element of the container transactionally. Calling
 * pop_back on an empty container does nothing. No iterators or
 * references except for back() and end() are invalidated.
 *
 * @post size() == std::max(0, size() - 1)
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::pop_back()
{
	if (empty())
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] { shrink(size() - 1); });
	assert(segment_capacity_validation());
}

/**
 * Resizes the container to count elements transactionally. If the
 * current size is greater than count, the container is reduced to its
 * first count elements. If the current size is less than count,
 * additional default-inserted elements are appended.
 *
 * @param[in] count new size of the container
 *
 * @post capacity() == nearest power of 2 greater than
 * std::max(capacity(), count)
 * @post size() == count
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::resize(size_type count)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		size_type _size = size();
		if (count < _size)
			shrink(count);
		else {
			if (capacity() < count)
				internal_reserve(count);
			construct(_size, count - _size);
		}
	});
	assert(segment_capacity_validation());
}

/**
 * Resizes the container to contain count elements transactionally. If
 * the current size is greater than count, the container is reduced to
 * its first count elements. If the current size is less than count,
 * additional copies of value are appended.
 *
 * @param[in] count new size of the container.
 * @param[in] value the value to initialize the new elements with.
 *
 * @post capacity() == nearest power of 2 greater than
 * std::max(capacity(), count)
 * @post size() == count
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::resize(size_type count,
					   const value_type &value)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		size_type _size = size();
		if (count < _size)
			shrink(count);
		else {
			if (capacity() < count)
				internal_reserve(count);
			construct(_size, count - _size, value);
		}
	});
	assert(segment_capacity_validation());
}

/**
 * Exchanges the contents of the container with other transactionally.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::swap(segment_vector &other)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		_data.swap(other._data);
		std::swap(_segments_used, other._segments_used);
	});
}

/**
 * Private helper method. Increases capacity.
 * Allocs new segments if segment where new_capacity located greater
 * than segment where located capacity().
 *
 * @pre must be called in transaction scope
 *
 * @param[in] new_capacity new desired capacity of the container.
 *
 * @post capacity() = nearest power of 2 larger than new_capacity
 *
 * @throw pmem::length_error when new_capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::internal_reserve(size_type new_capacity)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (new_capacity > max_size())
		throw std::length_error("New capacity exceeds max size.");

	if (new_capacity == 0)
		return;

	size_type old_idx = policy::get_segment(capacity());
	size_type new_idx = policy::get_segment(new_capacity - 1);
	storage::resize(_data, new_idx + 1);
	for (size_type i = old_idx; i <= new_idx; ++i) {
		size_type segment_capacity = policy::segment_size(i);
		_data[i].reserve(segment_capacity);
	}
	_segments_used = new_idx + 1;

	assert(segment_capacity_validation());
}

/**
 * Private helper function. Must be called during transaction. Assumes
 * that there is free space for additional elements. Constructs elements
 * at given index in underlying segments based on given parameters.
 *
 * @param[in] idx underlying segments index where new elements will be
 * constructed.
 * @param[in] count number of elements to be constructed.
 * @param[in] args variadic template arguments for value_type
 * constructor.
 *
 * @pre must be called in transaction scope.
 * @pre if initialized, range [end(), end() + count) must be snapshotted
 * in current transaction.
 * @pre capacity() >= size() + count
 * @pre args is valid argument for value_type constructor.
 *
 * @post size() == size() + count
 *
 * @throw rethrows constructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying segment in transaction failed.
 */
template <typename T, typename Segment, typename Policy>
template <typename... Args>
void
segment_vector<T, Segment, Policy>::construct(size_type idx, size_type count,
					      Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(_segments_used - 1 >= policy::get_segment(size() + count - 1));

	for (size_type i = idx; i < idx + count; ++i) {
		size_type segment = policy::get_segment(i);
		_data[segment].emplace_back(std::forward<Args>(args)...);
	}
}

/**
 * Private helper function. Must be called during transaction. Assumes
 * that there is free space for additional elements and input arguments
 * satisfy InputIterator requirements. Copies elements at index idx in
 * underlying segments with the contents of the range [first, last).
 * This overload participates in overload resolution only if InputIt
 * satisfies InputIterator.
 *
 * @param[in] idx underyling array index where new elements will be
 * constructed.
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @pre must be called in transaction scope.
 * @pre if initialized, range [end(), end() + std::distance(first,
 * last)) must be snapshotted in current transaction.
 * @pre capacity() >= size() + std::distance(first, last)
 * @pre InputIt is InputIterator.
 * @pre InputIt::reference is valid argument for value_type copy
 * constructor.
 *
 * @post size() == size() + std::distance(first, last)
 *
 * @throw rethrows constructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_out_of_memory when not enough memory to
 * allocate
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying segment in transaction failed.
 */
template <typename T, typename Segment, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
void
segment_vector<T, Segment, Policy>::construct_range(size_type idx,
						    InputIt first, InputIt last)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	size_type count = static_cast<size_type>(std::distance(first, last));
	assert(count >= 0);
	assert(_segments_used - 1 >= policy::get_segment(size() + count - 1));

	for (size_type i = idx; i < idx + count; ++i, ++first) {
		size_type segment = policy::get_segment(i);
		_data[segment].emplace_back(*first);
	}
}

/**
 * Private helper function. Must be called during transaction. Inserts a
 * gap for count elements starting at index idx. If there is not enough
 * space available, reallocation occurs.
 *
 * param[in] idx index number where gap should be made.
 * param[in] count length (expressed in number of elements) of the gap.
 *
 * @pre must be called in transaction scope.
 *
 * @post if there is not enough space for additional gap, new segment
 * will be allocated and capacity() will equal to nearest power of 2
 * greater than size()
 * + count.
 *
 * @throw rethrows constructor exception.
 * @throw pmem::length_error when new capacity larger than max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::insert_gap(size_type idx, size_type count)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	size_type _size = size();

	if (capacity() < _size + count)
		internal_reserve(_size + count);

	iterator dest = iterator(this, _size + count);
	iterator begin = iterator(this, idx);
	iterator end = iterator(this, _size);

	snapshot_data(idx, _size);

	resize(_size + count);
	std::move_backward(begin, end, dest);

	assert(segment_capacity_validation());
}

/**
 * Private helper function. Must be called during transaction. Destroys
 * elements in underlying array beginning from position size_new.
 *
 * @param[in] size_new new size
 *
 * @pre must be called in transaction scope.
 * @pre if initialized, range [begin(), end()) must be snapshotted in
 * current transaction.
 * @pre size_new <= size()
 *
 * @post size() == size_new
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing underlying segment
 * failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::shrink(size_type size_new)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size_new <= size());

	snapshot_data(size_new, size());

	size_type begin = policy::get_segment(size() - 1);
	size_type end = policy::get_segment(size_new);
	for (; begin > end; --begin) {
		_data[begin].clear();
	}
	size_type residue = policy::index_in_segment(size_new);
	_data[end].erase(_data[end].cbegin() + residue, _data[end].cend());
}

/**
 * Private helper function.
 *
 * @pre underlying segments must reside in persistent memory pool.
 *
 * @return reference to pool_base object where segment_vector resides.
 */
template <typename T, typename Segment, typename Policy>
pool_base
segment_vector<T, Segment, Policy>::get_pool() const noexcept
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);
	return pool_base(pop);
}

/**
 * Private helper function. Takes a “snapshot” of data in range
 * [this[first], this[last])
 *
 * @param[in] first first index.
 * @param[in] last last index.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename T, typename Segment, typename Policy>
void
segment_vector<T, Segment, Policy>::snapshot_data(size_type first,
						  size_type last)
{
	if (first == last)
		return;

	size_type segment = policy::get_segment(first);
	size_type end = policy::get_segment(last - 1);
	size_type count = policy::segment_top(segment + 1) - first;

	while (segment != end) {
		detail::conditional_add_to_tx(&cget(first), count,
					      POBJ_XADD_ASSUME_INITIALIZED);
		first = policy::segment_top(++segment);
		count = policy::segment_size(segment);
	}
	detail::conditional_add_to_tx(&cget(first), last - first,
				      POBJ_XADD_ASSUME_INITIALIZED);
}

/**
 * Private helper function. Not considering if element exist or not.
 *
 * @return reference to element with given index in segment_vector.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::reference
segment_vector<T, Segment, Policy>::get(size_type n)
{
	size_type s_idx = policy::get_segment(n);
	size_type local_idx = policy::index_in_segment(n);

	return _data[s_idx][local_idx];
}

/**
 * Private helper function. Not considering if element exist or not.
 *
 * @return const reference to element with given index.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
segment_vector<T, Segment, Policy>::get(size_type n) const
{
	size_type s_idx = policy::get_segment(n);
	size_type local_idx = policy::index_in_segment(n);

	return _data[s_idx][local_idx];
}

/**
 * Private helper function. Not considering if element exist or not.
 *
 * @return const reference to element with given index.
 */
template <typename T, typename Segment, typename Policy>
typename segment_vector<T, Segment, Policy>::const_reference
segment_vector<T, Segment, Policy>::cget(size_type n) const
{
	size_type s_idx = policy::get_segment(n);
	size_type local_idx = policy::index_in_segment(n);

	return _data[s_idx][local_idx];
}

/**
 * Private helper function. Checks if each allocated segment match its
 * expected capacity according to static_segment_policy.
 *
 * @return true if capacity of all segments matches to expected
 * capacity, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
segment_vector<T, Segment, Policy>::segment_capacity_validation() const
{
	for (size_type i = 0; i < _segments_used; ++i)
		if (_data.const_at(i).capacity() != policy::segment_size(i))
			return false;
	return true;
}

/**
 * Swaps the contents of lhs and rhs.
 *
 * @param[in] lhs first segment_vector.
 * @param[in] rhs second segment_vector.
 */
template <typename T, typename Segment, typename Policy>
void
swap(segment_vector<T, Segment, Policy> &lhs,
     segment_vector<T, Segment, Policy> &rhs)
{
	lhs.swap(rhs);
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each
 * element in lhs is equal to element in rhs at the same position.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of the containers are equal, false
 * otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator==(const segment_vector<T, Segment, Policy> &lhs,
	   const segment_vector<T, Segment, Policy> &rhs)
{
	return lhs.size() == rhs.size() &&
		std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the different number of elements or at
 * least one element in lhs is not equal to element in rhs at the same
 * position.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of the containers are not equal, false
 * otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator!=(const segment_vector<T, Segment, Policy> &lhs,
	   const segment_vector<T, Segment, Policy> &rhs)
{
	return !(lhs == rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of lhs are lexicographically less than
 * contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator<(const segment_vector<T, Segment, Policy> &lhs,
	  const segment_vector<T, Segment, Policy> &rhs)
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
					    rhs.end());
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 * @param[in] rhs is pmem::obj::experime ntal::segment_vector<T,
 * Policy>.
 *
 * @return true if contents of lhs are lexicographically lesser than or
 * equal to contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator<=(const segment_vector<T, Segment, Policy> &lhs,
	   const segment_vector<T, Segment, Policy> &rhs)
{
	return !(rhs < lhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of lhs are lexicographically greater than
 * contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator>(const segment_vector<T, Segment, Policy> &lhs,
	  const segment_vector<T, Segment, Policy> &rhs)
{
	return rhs < lhs;
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of lhs are lexicographically greater than or
 * equal to contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator>=(const segment_vector<T, Segment, Policy> &lhs,
	   const segment_vector<T, Segment, Policy> &rhs)
{
	return !(lhs < rhs);
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each
 * element in lhs is equal to element in rhs at the same position.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T,
 * SegmentPolicy, StoragePolicy>
 * @param[in] rhs is std::vector<T>.
 *
 * @return true if contents of the containers are equal, false
 * otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator==(const segment_vector<T, Segment, Policy> &lhs,
	   const std::vector<T> &rhs)
{
	return lhs.size() == rhs.size() &&
		std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each
 * element in lhs is equal to element in rhs at the same position.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T,
 * SegmentPolicy, StoragePolicy>
 * @param[in] rhs is std::vector<T>.
 *
 * @return true if contents of the containers are not equal, false
 * otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator!=(const segment_vector<T, Segment, Policy> &lhs,
	   const std::vector<T> &rhs)
{
	return !(lhs == rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T,
 * SegmentPolicy, StoragePolicy>
 * @param[in] rhs is std::vector<T>.
 *
 * @return true if contents of lhs are lexicographically less than
 * contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator<(const segment_vector<T, Segment, Policy> &lhs,
	  const std::vector<T> &rhs)
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
					    rhs.end());
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T,
 * SegmentPolicy, StoragePolicy>
 * @param[in] rhs is std::vector<T>.
 *
 * @return true if contents of lhs are lexicographically lesser than or
 * equal to contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator<=(const segment_vector<T, Segment, Policy> &lhs,
	   const std::vector<T> &rhs)
{
	return !(std::lexicographical_compare(rhs.begin(), rhs.end(),
					      lhs.begin(), lhs.end()));
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T,
 * SegmentPolicy, StoragePolicy>
 * @param[in] rhs is std::vector<T>.
 *
 * @return true if contents of lhs are lexicographically greater than
 * contents of rhs, false otherwise.
 */

template <typename T, typename Segment, typename Policy>
bool
operator>(const segment_vector<T, Segment, Policy> &lhs,
	  const std::vector<T> &rhs)
{
	return !(lhs <= rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_vector<T,
 * SegmentPolicy, StoragePolicy>
 * @param[in] rhs is std::vector<T>.
 *
 * @return true if contents of lhs are lexicographically greater than or
 * equal to contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator>=(const segment_vector<T, Segment, Policy> &lhs,
	   const std::vector<T> &rhs)
{
	return !(lhs < rhs);
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each
 * element in lhs is equal to element in rhs at the same position.
 *
 * @param[in] lhs is std::vector<T>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of the containers are equal, false
 * otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator==(const std::vector<T> &lhs,
	   const segment_vector<T, Segment, Policy> &rhs)
{
	return rhs == lhs;
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each
 * element in lhs is equal to element in rhs at the same position.
 *
 * @param[in] lhs is std::vector<T>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of the containers are not equal, false
 * otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator!=(const std::vector<T> &lhs,
	   const segment_vector<T, Segment, Policy> &rhs)
{
	return !(lhs == rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is std::vector<T>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of lhs are lexicographically less than
 * contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator<(const std::vector<T> &lhs,
	  const segment_vector<T, Segment, Policy> &rhs)
{
	return rhs > lhs;
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is std::vector<T>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of lhs are lexicographically lesser than or
 * equal to contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator<=(const std::vector<T> &lhs,
	   const segment_vector<T, Segment, Policy> &rhs)
{
	return !(rhs < lhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is std::vector<T>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of lhs are lexicographically greater than
 * contents of rhs, false otherwise.
 */

template <typename T, typename Segment, typename Policy>
bool
operator>(const std::vector<T> &lhs,
	  const segment_vector<T, Segment, Policy> &rhs)
{
	return rhs < lhs;
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is std::vector<T>.
 * @param[in] rhs is pmem::obj::experimental::segment_vector<T, Policy,
 * SPolicy>.
 *
 * @return true if contents of lhs are lexicographically greater than or
 * equal to contents of rhs, false otherwise.
 */
template <typename T, typename Segment, typename Policy>
bool
operator>=(const std::vector<T> &lhs,
	   const segment_vector<T, Segment, Policy> &rhs)
{
	return !(lhs < rhs);
}

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */

#endif /* LIBPMEMOBJ_SEGMENT_VECTOR_HPP */
