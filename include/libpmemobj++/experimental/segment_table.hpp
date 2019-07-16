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
 * A persistent version of segment table implementation.
 */

#ifndef PMEMOBJ_SEGMENT_TABLE_HPP
#define PMEMOBJ_SEGMENT_TABLE_HPP

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/detail/temp_value.hpp>
#include <libpmemobj++/experimental/contiguous_iterator.hpp>
#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj.h>

#include <type_traits>
#include <vector>

namespace pmem
{
namespace obj
{

template <typename T>
class segment_policy {
public:
	/* Traits */
	using value_type = T;
	using size_type = std::size_t;

	/**
	 * @return index of segment where should locate element with specified
	 * index.
	 */
	static constexpr size_type
	get_segment(size_type idx)
	{
		return static_cast<size_type>(detail::Log2(idx | 1));
	}

	/**
	 * @return index of first element in specified segment index.
	 */
	static constexpr size_type
	segment_top(size_type segment_idx)
	{
		return (size_type(1) << segment_idx) & ~size_type(1);
	}

	/**
	 * @return return size for specified segment index.
	 */
	static constexpr size_type
	segment_size(size_type segment_idx)
	{
		return (segment_idx == 0) ? 2 : segment_top(segment_idx);
	}

	/**
	 * @return item number in the segment in which it should be.
	 */
	static constexpr size_type
	segment_local(size_type idx)
	{
		return idx - segment_top(get_segment(idx));
	}

	static constexpr size_type
	largest_segment()
	{
		return get_segment(PMEMOBJ_MAX_ALLOC_SIZE / sizeof(value_type));
	}

	static constexpr size_type
	max_size()
	{
		return 2 * segment_size(largest_segment());
	}

	static const long unsigned int MAX_SEGMENTS = largest_segment() + 1;
};

namespace experimental
{

/* Iterator for segment_table */
template <typename Container, bool is_const>
class segment_iterator {
public:
	using iterator_category = std::random_access_iterator_tag;
	using difference_type = std::ptrdiff_t;
	using table_type = Container;
	using size_type = typename table_type::size_type;
	using value_type = typename table_type::value_type;
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

	friend class segment_iterator<Container, true>;
	friend class segment_iterator<Container, false>;

private:
	table_ptr table; // pointer to container for iteration
	size_type index; // index to container element when itarator is

public:
	/* constructors */
	segment_iterator() noexcept;
	explicit segment_iterator(table_ptr tab, size_type idx) noexcept;

	/* Copy constructor to enable conversion from non-const to const
	 * iterator*/
	template <typename U = void,
		  typename = typename std::enable_if<is_const, U>::type>
	segment_iterator(const segment_iterator<Container, false> &other);

	/* in(de)crement methods */
	segment_iterator &operator++();
	segment_iterator operator++(int);
	segment_iterator operator+(difference_type idx) const;
	segment_iterator &operator+=(difference_type idx);
	segment_iterator &operator--();
	segment_iterator operator--(int);
	segment_iterator operator-(difference_type idx) const;
	segment_iterator &operator-=(difference_type idx);
	template <bool C>
	difference_type operator+(const segment_iterator<Container, C> &rhs);
	template <bool C>
	difference_type operator-(const segment_iterator<Container, C> &rhs);

	/* compare methods */
	template <bool C>
	bool operator==(const segment_iterator<Container, C> &rhs);
	template <bool C>
	bool operator!=(const segment_iterator<Container, C> &rhs);
	template <bool C>
	bool operator<(const segment_iterator<Container, C> &rhs);
	template <bool C>
	bool operator>(const segment_iterator<Container, C> &rhs);
	template <bool C>
	bool operator<=(const segment_iterator<Container, C> &rhs);
	template <bool C>
	bool operator>=(const segment_iterator<Container, C> &rhs);

	/* access methods */
	reference operator*() const;
	pointer operator->() const;
};

template <typename T, typename Policy = segment_policy<T>>
class segment_table {
public:
	/* Traits */
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using iterator = segment_iterator<segment_table, false>;
	using const_iterator = segment_iterator<segment_table, true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	/* Constructors */
	segment_table();
	segment_table(size_type count, const value_type &value);
	explicit segment_table(size_type count);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	segment_table(InputIt first, InputIt last);
	segment_table(const segment_table &other);
	segment_table(segment_table &&other);
	segment_table(std::initializer_list<T> init);
	segment_table(const std::vector<T> &other);

	/* Assign operators */
	segment_table &operator=(const segment_table &other);
	segment_table &operator=(segment_table &&other);
	segment_table &operator=(std::initializer_list<T> ilist);
	segment_table &operator=(const std::vector<T> &other);

	/* Assign methods */
	void assign(size_type count, const T &value);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	void assign(InputIt first, InputIt last);
	void assign(std::initializer_list<T> ilist);
	void assign(const segment_table &other);
	void assign(segment_table &&other);
	void assign(const std::vector<T> &other);

	/* Destructor */
	~segment_table();

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
	void swap(segment_table &other);

private:
	/* helper functions */
	void internal_reserve(size_type new_capacity);
	void dealloc();
	template <typename... Args>
	void construct(size_type idx, size_type count, Args &&... args);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	void construct_range(size_type idx, InputIt first, InputIt last);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	void construct_range_copy(size_type idx, InputIt first, InputIt last);
	void insert_gap(size_type idx, size_type count);
	void shrink(size_type size_new);
	void check_pmem();
	void check_tx_stage_work();
	pool_base get_pool() const noexcept;
	void snapshot_data(size_type idx_first, size_type idx_last);

	/* data structure specific helper functions */
	reference get(size_type n);
	const_reference get(size_type n) const;
	bool segment_capacity_validation();

	p<size_type> _size = 0;
	p<size_type> _capacity = 0;

	using segment_t = pmem::obj::experimental::vector<T>;
	/* Underlying segments */
	segment_t _data[Policy::MAX_SEGMENTS];
};

/* Non-member swap */
template <typename T, typename Policy>
void swap(segment_table<T, Policy> &lhs, segment_table<T, Policy> &rhs);

/*
 * Comparison operators between pmem::obj::experimental::segment_table<T,
 * Policy> and pmem::obj::experimental::segment_table<T, Policy>
 */
template <typename T, typename Policy>
bool operator==(const segment_table<T, Policy> &lhs,
		const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator!=(const segment_table<T, Policy> &lhs,
		const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator<(const segment_table<T, Policy> &lhs,
	       const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator<=(const segment_table<T, Policy> &lhs,
		const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator>(const segment_table<T, Policy> &lhs,
	       const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator>=(const segment_table<T, Policy> &lhs,
		const segment_table<T, Policy> &rhs);

/*
 * Comparison operators between pmem::obj::experimental::segment_table<T,
 * Policy> and std::vector<T>
 */
template <typename T, typename Policy>
bool operator==(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs);
template <typename T, typename Policy>
bool operator!=(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs);
template <typename T, typename Policy>
bool operator<(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs);
template <typename T, typename Policy>
bool operator<=(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs);
template <typename T, typename Policy>
bool operator>(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs);
template <typename T, typename Policy>
bool operator>=(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs);

/*
 * Comparison operators between std::vector<T> and
 * pmem::obj::experimental::segment_table<T, Policy>
 */
template <typename T, typename Policy>
bool operator==(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator!=(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator<(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator<=(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator>(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs);
template <typename T, typename Policy>
bool operator>=(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs);

/**
 * Default constructor. Constructs an empty container.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 */
template <typename T, typename Policy>
segment_table<T, Policy>::segment_table()
{
	check_pmem();
	check_tx_stage_work();
}

/**
 * Constructs the container with count copies of elements with value value.
 *
 * @param[in] count number of elements to construct.
 * @param[in] value value of all constructed elements.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == count
 * @post capacity() == size()
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error if allocating memory for segment in
 * transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 * @throw rethrows vector reserve() exception.
 * @throw rethrows vector emplace_back() exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy>::segment_table(size_type count,
					const value_type &value)
{
	check_pmem();
	check_tx_stage_work();

	internal_reserve(count);
	construct(0, count, value);
}

/**
 * Constructs the container with count copies of T default constructed values.
 *
 * @param[in] count number of elements to construct.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == count
 * @post capacity() == size()
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error if allocating memory for segment in
 * transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 * @throw rethrows vector reserve() exception.
 * @throw rethrows vector emplace_back() exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy>::segment_table(size_type count)
{
	check_pmem();
	check_tx_stage_work();

	internal_reserve(count);
	construct(0, count);
}

/**
 * Constructs the container with the contents of the range [first, last). The
 * first and last arguments must satisfy InputIterator requirements. This
 * overload only participates in overload resolution if InputIt satisfies
 * InputIterator, to avoid ambiguity with the overload of count-value
 * constructor.
 *
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == std::distance(first, last)
 * @post capacity() == size()
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error if allocating memory for segment in
 * transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 * @throw rethrows vector reserve() exception.
 * @throw rethrows vector emplace_back() exception.
 */
template <typename T, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
segment_table<T, Policy>::segment_table(InputIt first, InputIt last)
{
	check_pmem();
	check_tx_stage_work();

	internal_reserve(static_cast<size_type>(std::distance(first, last)));
	construct_range_copy(0, first, last);
}

/**
 * Copy constructor. Constructs the container with the copy of the
 * contents of other.
 *
 * @param[in] other reference to the segment_table to be copied.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error if allocating memory for segment in
 * transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 * @throw rethrows vector reserve() exception.
 * @throw rethrows vector emplace_back() exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy>::segment_table(const segment_table &other)
{
	check_pmem();
	check_tx_stage_work();

	internal_reserve(other.capacity());
	construct_range_copy(0, other.cbegin(), other.cend());
}

/**
 * Move constructor. Constructs the container with the contents of other using
 * move semantics. After the move, other is guaranteed to be empty().
 *
 * @param[in] other rvalue reference to the segment_table to be moved from.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 * @post data == other.data
 * @post other.data[i] == nullptr
 * @post other.capacity() == 0
 * @post other.size() == 0
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows vector free_data() exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy>::segment_table(segment_table &&other)
{
	check_pmem();
	check_tx_stage_work();

	_capacity = other._capacity;
	_size = other._size;

	size_type end = Policy::get_segment(_capacity);
	for (size_type i = 0; i <= end; ++i) {
		_data[i] = other._data[i];
		other._data[i].free_data();
	}

	other._capacity = other._size = 0;
}

/**
 * Constructs the container with the contents of the initializer list init.
 *
 * @param[in] init initializer list with content to be constructed.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == init.size()
 * @post capacity() == size()
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error if allocating memory for segment in
 * transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 * @throw rethrows vector reserve() exception.
 * @throw rethrows vector emplace_back() exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy>::segment_table(std::initializer_list<T> init)
    : segment_table(init.begin(), init.end())
{
}

/**
 * Copy constructor. Constructs the container with the copy of the contents of
 * std::vector<T> other.
 *
 * @param[in] other reference to the vector to be copied.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error if allocating memory for segment in
 * transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 * @throw rethrows vector reserve() exception.
 * @throw rethrows vector emplace_back() exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy>::segment_table(const std::vector<T> &other)
    : segment_table(other.cbegin(), other.cend())
{
}

/**
 * Copy assignment operator. Replaces the contents with a copy of the contents
 * of other transactionally.
 *
 * @post size() == other.size()
 * @post capacity() == max(size(), other.capacity())
 *
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy> &
segment_table<T, Policy>::operator=(const segment_table &other)
{
	assign(other);
	return *this;
}

/**
 * Move assignment operator. Replaces the contents with those of other using
 * move semantics (i.e. the data in other is moved from other into this
 * container) transactionally. Other is in a valid but empty state afterwards.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw pmem::transaction_free_error when freeing underlying segments failed.
 */
template <typename T, typename Policy>
segment_table<T, Policy> &
segment_table<T, Policy>::operator=(segment_table &&other)
{
	assign(std::move(other));
	return *this;
}

/**
 * Replaces the contents with those identified by initializer list ilist
 * transactionally.
 *
 * @throw std::length_error if ilist.size() > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy> &
segment_table<T, Policy>::operator=(std::initializer_list<T> ilist)
{
	assign(ilist.begin(), ilist.end());
	return *this;
}

/**
 * Copy assignment operator. Replaces the contents with a copy of the contents
 * of std::vector<T> other transactionally.
 *
 * @post size() == other.size()
 * @post capacity() == max(size(), other.capacity())
 *
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
segment_table<T, Policy> &
segment_table<T, Policy>::operator=(const std::vector<T> &other)
{
	assign(other);
	return *this;
}

/**
 * Replaces the contents with count copies of value value transactionally. All
 * iterators, pointers and references to the elements of the container are
 * invalidated. The past-the-end iterator is also invalidated.
 *
 * @param[in] count number of elements to construct.
 * @param[in] value value of all constructed elements.
 *
 * @post size() == count
 * @post capacity() == max(size(), count)
 *
 * @throw std::length_error if count > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::assign(size_type count, const_reference value)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (count > _capacity)
			internal_reserve(count);

		size_type last_segment = Policy::get_segment(count - 1);
		for (size_type i = 0; i < last_segment; ++i) {
			_data[i].assign(Policy::segment_size(i), value);
		}
		_data[last_segment].assign(Policy::segment_local(count), value);

		if (count < _size)
			shrink(count);

		_size = count;
	});
	assert(segment_capacity_validation());
}

/**
 * Replaces the contents with copies of those in the range [first, last)
 * transactionally. This overload participates in overload resolution only if
 * InputIt satisfies InputIterator. All iterators, pointers and references to
 * the elements of the container are invalidated. The past-the-end iterator is
 * also invalidated.
 *
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @post size() == std::distance(first, last)
 * @post capacity() == max(size(), std::distance(first, last))
 *
 * @throw std::length_error if std::distance(first, last) > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 * @throw rethrows vector assign() exception.
 */
template <typename T, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
void
segment_table<T, Policy>::assign(InputIt first, InputIt last)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		size_type count =
			static_cast<size_type>(std::distance(first, last));
		if (count > _capacity)
			internal_reserve(count);

		difference_type num;
		size_type end = Policy::get_segment(count - 1);
		for (size_type i = 0; i < end; ++i) {
			num = static_cast<difference_type>(
				Policy::segment_size(i));
			_data[i].assign(first, std::next(first, num));
			std::advance(first, num);
		}
		num = static_cast<difference_type>(std::distance(first, last));
		_data[end].assign(first, std::next(first, num));

		if (count < _size)
			shrink(count);

		_size = count;
	});
	assert(segment_capacity_validation());
}

/**
 * Replaces the contents with the elements from the initializer list ilist
 * transactionally. All iterators, pointers and references to the elements of
 * the container are invalidated. The past-the-end iterator is also invalidated.
 *
 * @param[in] ilist initializer list with content to be constructed.
 *
 * @post size() == std::distance(ilist.begin(), ilist.end())
 * @post capacity() == max(size(), capacity())
 *
 * @throw std::length_error if std::distance(first, last) > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::assign(std::initializer_list<T> ilist)
{
	assign(ilist.begin(), ilist.end());
}

/**
 * Copy assignment method. Replaces the contents with a copy of the contents
 * of other transactionally.
 *
 * @post size() == other.size()
 * @post capacity() == max(size(), other.capacity())
 *
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::assign(const segment_table &other)
{
	if (this != &other)
		assign(other.cbegin(), other.cend());
}

/**
 * Move assignment method. Replaces the contents with those of other using
 * move semantics (i.e. the data in other is moved from other into this
 * container) transactionally. Other is in a valid but empty state afterwards.
 * This method is not specified by STL standards.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw pmem::transaction_free_error when freeing underlying segments failed.
 * @throw rethrows vector free_data() exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::assign(segment_table &&other)
{
	if (this == &other)
		return;

	pool_base pb = get_pool();

	transaction::run(pb, [&] {
		shrink(0);

		_capacity = other._capacity;
		_size = other._size;

		size_type end = Policy::get_segment(_capacity);
		for (size_type i = 0; i <= end; ++i) {
			_data[i] = other._data[i];
			other._data[i].free_data();
		}

		other._capacity = other._size = 0;
	});
}

/**
 * Copy assignment method. Replaces the contents with a copy of the contents
 * of std::vector<T> other transactionally.
 *
 * @post size() == other.size()
 * @post capacity() == max(size(), other.capacity())
 *
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::assign(const std::vector<T> &other)
{
	assign(other.cbegin(), other.cend());
}

/**
 * Destructor.
 * Note that free_data may throw an transaction_free_error when freeing
 * underlying segments failed. It is recommended to call free_data manually
 * before object destruction.
 *
 * @throw rethrows destructor exception.
 * @throw transaction_free_error when freeing underlying segments failed.
 */
template <typename T, typename Policy>
segment_table<T, Policy>::~segment_table()
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
 * @throw std::out_of_range if n is not within the range of the container.
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::reference
segment_table<T, Policy>::at(size_type n)
{
	if (n >= _size)
		throw std::out_of_range("segment_table::at");

	detail::conditional_add_to_tx(&get(n));

	return get(n);
}

/**
 * Access element at specific index with bounds checking.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying segments.
 *
 * @throw std::out_of_range if n is not within the range of the container.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reference
segment_table<T, Policy>::at(size_type n) const
{
	if (n >= _size)
		throw std::out_of_range("segment_table::at");
	return get(n);
}

/**
 * Access element at specific index with bounds checking. In contradiction to
 * at(), const_at() will return const_reference not depending on the
 * const-qualification of the object it is called on.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying segments.
 *
 * @throw std::out_of_range if n is not within the range of the container.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reference
segment_table<T, Policy>::const_at(size_type n) const
{
	if (n >= _size)
		throw std::out_of_range("segment_table::const_at");
	return get(n);
}

/**
 * Access element at specific index and add it to a transaction. No bounds
 * checking is performed.
 *
 * @param[in] n index number.
 *
 * @return reference to element number n in underlying segments.
 *
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::reference segment_table<T, Policy>::
operator[](size_type n)
{
	reference element = get(n);

	detail::conditional_add_to_tx(&element);

	return element;
}

/**
 * Access element at specific index. No bounds checking is performed.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying segments.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reference segment_table<T, Policy>::
operator[](size_type n) const
{
	return get(n);
}

/**
 * Access the first element and add this element to a transaction.
 *
 * @return reference to first element in underlying segments.
 *
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::reference
segment_table<T, Policy>::front()
{
	detail::conditional_add_to_tx(&_data[0][0]);

	return _data[0][0];
}

/**
 * Access the first element.
 *
 * @return const_reference to first element in underlying segments.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reference
segment_table<T, Policy>::front() const
{
	return _data[0][0];
}

/**
 * Access the first element. In contradiction to front(), cfront() will return
 * const_reference not depending on the const-qualification of the object it is
 * called on.
 *
 * @return reference to first element in underlying segments.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reference
segment_table<T, Policy>::cfront() const
{
	return _data[0][0];
}

/**
 * Access the last element and add this element to a transaction.
 *
 * @return reference to the last element in underlying segments.
 *
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::reference
segment_table<T, Policy>::back()
{
	reference element = get(_size - 1);

	detail::conditional_add_to_tx(&element);

	return element;
}

/**
 * Access the last element.
 *
 * @return const_reference to the last element in underlying segments.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reference
segment_table<T, Policy>::back() const
{
	return get(_size - 1);
}

/**
 * Access the last element. In contradiction to back(), cback() will return
 * const_reference not depending on the const-qualification of the object it is
 * called on.
 *
 * @return const_reference to the last element in underlying segments.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reference
segment_table<T, Policy>::cback() const
{
	return get(_size - 1);
}

/**
 * Returns an iterator to the beginning.
 *
 * @return iterator pointing to the first element in the segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::begin()
{
	return iterator(this, 0);
}

/**
 * Returns const iterator to the beginning.
 *
 * @return const_iterator pointing to the first element in the segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_iterator
segment_table<T, Policy>::begin() const noexcept
{
	return const_iterator(this, 0);
}

/**
 * Returns const iterator to the beginning. In contradiction to begin(),
 * cbegin() will return const_iterator not depending on the const-qualification
 * of the object it is called on.
 *
 * @return const_iterator pointing to the first element in the segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_iterator
segment_table<T, Policy>::cbegin() const noexcept
{
	return const_iterator(this, 0);
}

/**
 * Returns an iterator to past the end.
 *
 * @return iterator referring to the past-the-end element in the segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::end()
{
	return iterator(this, _size);
}

/**
 * Returns a const iterator to past the end.
 *
 * @return const_iterator referring to the past-the-end element in the
 * segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_iterator
segment_table<T, Policy>::end() const noexcept
{
	return const_iterator(this, _size);
}

/**
 * Returns a const iterator to the end. In contradiction to end(), cend() will
 * return const_iterator not depending on the const-qualification of the object
 * it is called on.
 *
 * @return const_iterator referring to the past-the-end element in the
 * segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_iterator
segment_table<T, Policy>::cend() const noexcept
{
	return const_iterator(this, _size);
}

/**
 * Returns a reverse iterator to the beginning.
 *
 * @return reverse_iterator pointing to the last element in the segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::reverse_iterator
segment_table<T, Policy>::rbegin()
{
	return reverse_iterator(end());
}

/**
 * Returns a const reverse iterator to the beginning.
 *
 * @return const_reverse_iterator pointing to the last element in the
 * segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reverse_iterator
segment_table<T, Policy>::rbegin() const noexcept
{
	return const_reverse_iterator(end());
}

/**
 * Returns a const reverse iterator to the beginning. In contradiction to
 * rbegin(), crbegin() will return const_reverse_iterator not depending on the
 * const-qualification of the object it is called on.
 *
 * @return const_reverse_iterator pointing to the last element in the
 * segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reverse_iterator
segment_table<T, Policy>::crbegin() const noexcept
{
	return rbegin();
}

/**
 * Returns a reverse iterator to the end.
 *
 * @return reverse_iterator pointing to the theoretical element preceding the
 * first element in the segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::reverse_iterator
segment_table<T, Policy>::rend()
{
	return reverse_iterator(begin());
}

/**
 * Returns a const reverse iterator to the end.
 *
 * @return const_reverse_iterator pointing to the theoretical element preceding
 * the first element in the segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reverse_iterator
segment_table<T, Policy>::rend() const noexcept
{
	return const_reverse_iterator(begin());
}

/**
 * Returns a const reverse iterator to the beginning. In contradiction to
 * rend(), crend() will return const_reverse_iterator not depending on the
 * const-qualification of the object it is called on.
 *
 * @return const_reverse_iterator pointing to the theoretical element preceding
 * the first element in the segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reverse_iterator
segment_table<T, Policy>::crend() const noexcept
{
	return rend();
}

/**
 * Checks whether the container is empty.
 *
 * @return true if container is empty, false otherwise.
 */
template <typename T, typename Policy>
constexpr bool
segment_table<T, Policy>::empty() const noexcept
{
	return _size == 0;
}

/**
 * @return number of elements.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::size_type
segment_table<T, Policy>::size() const noexcept
{
	return _size;
}

/**
 * @return maximum number of elements the container is able to hold due to PMDK
 * limitations.
 */
template <typename T, typename Policy>
constexpr typename segment_table<T, Policy>::size_type
segment_table<T, Policy>::max_size() const noexcept
{
	return Policy::max_size();
}

/**
 * Increases the capacity of the segment_table to capacity_new transactionally.
 * If segment where should be capacity_new is greater than the current
 * capacity's segment, new segments allocated, otherwise the method does
 * nothing. If new segments allocated, all iterators, including the past-the-end
 * iterator, and all references to the elements are invalidated. Otherwise, no
 * iterators or references are invalidated.
 *
 * @param[in] capacity_new new capacity.
 *
 * @post capacity() == max(capacity(), capacity_new) -> segments
 *
 * @throw rethrows destructor exception.
 * @throw std::length_error if new_cap > max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::reserve(size_type capacity_new)
{
	if (capacity_new <= _capacity)
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] { internal_reserve(capacity_new); });
}

/**
 * @return number of elements that can be held in currently allocated storage
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::size_type
segment_table<T, Policy>::capacity() const noexcept
{
	return _capacity;
}

/**
 * Requests transactional removal of unused capacity. New capacity will be set
 * to current vector size. If reallocation occurs, all iterators, including the
 * past the end iterator, and all references to the elements are invalidated.
 * If no reallocation takes place, no iterators or references are invalidated.
 *
 * @post capacity() == closest power of two bigger than size()
 *
 * @throw pmem::transaction_error when snapshotting failed
 * @throw pmem::transaction_alloc_error when reallocating failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::shrink_to_fit()
{
	size_type capacity_new = _size;
	if (_capacity == capacity_new)
		return;

	pool_base pb = get_pool();
	size_type last_segment_new = Policy::get_segment(capacity_new - 1);
	size_type last_segment = Policy::get_segment(_capacity - 1);
	transaction::run(pb, [&] {
		for (size_type i = last_segment_new + 1; i <= last_segment; ++i)
			_data[i].free_data();
		_capacity = Policy::segment_top(last_segment_new + 1);
	});
}

/**
 * Clears the content of a segment_table transactionally.
 *
 * @post size() == 0
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::clear()
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] { shrink(0); });
}

/**
 * Clears the content of a segment_table and frees all allocated persistent
 * memory for data transactionally.
 *
 * @post size() == 0
 * @post capacity() == 0
 * @post each segment == nullptr
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing underlying array failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::free_data()
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] { dealloc(); });
}

/**
 * Inserts value before pos in the container transactionally. Causes
 * reallocation if the new size() is greater than the old capacity(). If the new
 * size() is greater than capacity(), all iterators and references are
 * invalidated. Otherwise, only the iterators and references before the
 * insertion point remain valid. The past-the-end iterator is also invalidated.
 *
 * @param[in] pos iterator before which the content will be inserted. pos may be
 * the end() iterator.
 * @param[in] value element value to be inserted.
 *
 * @return Iterator pointing to the inserted value.
 *
 * @pre value_type must meet the requirements of CopyAssignable and
 * CopyInsertable.
 *
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity, or remains the same if there is enough space to add single element.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::insert(const_iterator pos, const T &value)
{
	return insert(pos, 1, value);
}

/**
 * Moves value before pos in the container transactionally. Causes reallocation
 * if the new size() is greater than the old capacity(). If the new size() is
 * greater than capacity(), all iterators and references are invalidated.
 * Otherwise, only the iterators and references before the insertion point
 * remain valid. The past-the-end iterator is also invalidated.
 *
 * @param[in] pos iterator before which the content will be inserted. pos may be
 * the end() iterator.
 * @param[in] value element value to be inserted.
 *
 * @return Iterator pointing to the inserted value.
 *
 * @pre value_type must meet the requirements of MoveAssignable and
 * MoveInsertable.
 *
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity, or remains the same if there is enough space to add single element.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::insert(const_iterator pos, T &&value)
{
	pool_base pb = get_pool();

	size_type idx = static_cast<size_type>(pos - cbegin());

	transaction::run(pb, [&] {
		insert_gap(idx, 1);
		this->get(idx) = std::move(value);
	});

	return iterator(this, idx);
}

/**
 * Inserts count copies of the value before pos in the container
 * transactionally. Causes reallocation if the new size() is greater than the
 * old capacity(). If the new size() is greater than capacity(), all iterators
 * and references are invalidated. Otherwise, only the iterators and references
 * before the insertion point remain valid. The past-the-end iterator is also
 * invalidated.
 *
 * @param[in] pos iterator before which the content will be inserted. pos may be
 * the end() iterator.
 * @param[in] count number of copies to be inserted.
 * @param[in] value element value to be inserted.
 *
 * @return Iterator pointing to the first element inserted, or pos if count ==
 * 0.
 *
 * @pre value_type must meet the requirements of CopyAssignable and
 * CopyInsertable.
 *
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity + count, or remains the same if there is enough space to add count
 * elements.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::insert(const_iterator pos, size_type count,
				 const T &value)
{
	pool_base pb = get_pool();

	size_type idx = static_cast<size_type>(pos - cbegin());

	transaction::run(pb, [&] {
		insert_gap(idx, count);
		for (size_type i = idx; i < idx + count; ++i) {
			this->get(i) = std::move(value);
		}
	});

	return iterator(this, idx);
}

/**
 * Inserts elements from range [first, last) before pos in the container
 * transactionally. Causes reallocation if the new size() is greater than the
 * old capacity(). If the new size() is greater than capacity(), all iterators
 * and references are invalidated. Otherwise, only the iterators and references
 * before the insertion point remain valid. The past-the-end iterator is also
 * invalidated. This overload participates in overload resolution only if
 * InputIt qualifies as InputIterator, to avoid ambiguity with the
 * pos-count-value overload. The behavior is undefined if first and last are
 * iterators into *this.
 *
 * @param[in] pos iterator before which the content will be inserted. pos may be
 * the end() iterator.
 * @param[in] first begin of the range of elements to insert, can't be iterator
 * into container for which insert is called.
 * @param[in] last end of the range of elements to insert, can't be iterator
 * into container for which insert is called.
 *
 * @return Iterator pointing to the first element inserted or pos if first ==
 * last.
 *
 * @pre value_type must meet the requirements of EmplaceConstructible,
 * Swappable, CopyAssignable, CopyConstructible and CopyInsertable.
 * @pre InputIt must satisfies requirements of InputIterator.
 *
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity + std::distance(first, last), or remains the same if there is enough
 * space to add std::distance(first, last) elements.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::insert(const_iterator pos, InputIt first,
				 InputIt last)
{
	pool_base pb = get_pool();

	size_type idx = static_cast<size_type>(pos - cbegin());
	size_type gap_size = static_cast<size_type>(std::distance(first, last));

	transaction::run(pb, [&] {
		insert_gap(idx, gap_size);
		for (size_type i = idx; i < idx + gap_size; ++i, ++first) {
			this->get(i) = *first;
		}
	});

	return iterator(this, idx);
}

/**
 * Inserts elements from initializer list ilist before pos in the container
 * transactionally. Causes reallocation if the new size() is greater than the
 * old capacity(). If the new size() is greater than capacity(), all iterators
 * and references are invalidated. Otherwise, only the iterators and references
 * before the insertion point remain valid. The past-the-end iterator is also
 * invalidated.
 *
 * @param[in] pos iterator before which the content will be inserted. pos may be
 * the end() iterator.
 * @param[in] ilist initializer list to insert the values from.
 *
 * @return Iterator pointing to the first element inserted, or pos if ilist is
 * empty.
 *
 * @pre value_type must meet the requirements of EmplaceConstructible,
 * Swappable, CopyAssignable, CopyConstructible and CopyInsertable.
 *
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity + std::distance(ilist.begin(), ilist.end()), or remains the same if
 * there is enough space to add std::distance(ilist.begin(), ilist.end())
 * elements.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::insert(const_iterator pos,
				 std::initializer_list<T> ilist)
{
	return insert(pos, ilist.begin(), ilist.end());
}

/**
 * Inserts a new element into the container directly before pos. The element is
 * constructed in-place. The arguments args... are forwarded to the constructor
 * as std::forward<Args>(args).... If the new size() is greater than capacity(),
 * all iterators and references are invalidated. Otherwise, only the iterators
 * and references before the insertion point remain valid. The past-the-end
 * iterator is also invalidated. Note that standard allows args to be a self
 * reference and internal emplace implementation handles this case by creating
 * temporary element_type object. This object is being stored either on stack or
 * on pmem, see pmem::detail::temp_value for details.
 *
 * @param[in] pos iterator before which the new element will be constructed.
 * @param[in] args arguments to forward to the constructor of the element.
 *
 * @return Iterator pointing to the emplaced element.
 *
 * @pre value_type must meet the requirements of MoveAssignable, MoveInsertable
 * and EmplaceConstructible.
 *
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity, or remains the same if there is enough space to add elements.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
template <class... Args>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::emplace(const_iterator pos, Args &&... args)
{
	pool_base pb = get_pool();

	size_type idx = static_cast<size_type>(pos - cbegin());

	transaction::run(pb, [&] {
		detail::temp_value<value_type,
				   noexcept(T(std::forward<Args>(args)...))>
		tmp(std::forward<Args>(args)...);
		insert_gap(idx, 1);
		this->get(idx) = std::move(tmp.get());
	});

	return iterator(this, idx);
}

/**
 * Appends a new element to the end of the container. The element is constructed
 * in-place. The arguments args... are forwarded to the constructor as
 * std::forward<Args>(args).... If the new size() is greater than capacity()
 * then all iterators and references (including the past-the-end iterator) are
 * invalidated. Otherwise only the past-the-end iterator is invalidated.
 *
 * @param[in] args arguments to forward to the constructor of the element.
 *
 * @return Iterator pointing to the emplaced element.
 *
 * @pre value_type must meet the requirements of MoveInsertable and
 * EmplaceConstructible.
 *
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity, or remains the same if there is enough space to add single element.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
template <class... Args>
typename segment_table<T, Policy>::reference
segment_table<T, Policy>::emplace_back(Args &&... args)
{
	assert(_size <= max_size());

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (_size == _capacity)
			internal_reserve(_capacity + 1);

		size_type segment = Policy::get_segment(_size);
		_data[segment].emplace_back(std::forward<Args>(args)...);
		++_size;
	});
	return back();
}

/**
 * Removes the element at pos. Invalidates iterators and references at or after
 * the point of the erase, including the end() iterator. The iterator pos must
 * be valid and dereferenceable. Thus the end() iterator (which is valid, but is
 * not dereferenceable) cannot be used as a value for pos.
 *
 * @param[in] pos iterator to the element to be removed.
 *
 * @return Iterator following the last removed element. If the iterator pos
 * refers to the last element, the end() iterator is returned.
 *
 * @pre value_type must meet the requirements of MoveAssignable.
 *
 * @post size() = size() - 1.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::erase(const_iterator pos)
{
	return erase(pos, pos + 1);
}

/**
 * Removes the elements in the range [first, last). Invalidates iterators and
 * references at or after the point of the erase, including the end() iterator.
 * The iterator pos must be valid and dereferenceable. Thus the end() iterator
 * (which is valid, but is not dereferenceable) cannot be used as a value for
 * pos.
 *
 * @param[in] first beginning of the range of elements to be removed.
 * @param[in] last end of range of elements to be removed.
 *
 * @return Iterator following the last removed element. If the iterator pos
 * refers to the last element, the end() iterator is returned. If first and last
 * refer to the same element, iterator to this element is returned.
 *
 * @pre value_type must meet the requirements of MoveAssignable.
 *
 * @post size() = size() - std::distance(first, last).
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::iterator
segment_table<T, Policy>::erase(const_iterator first, const_iterator last)
{
	size_type count = static_cast<size_type>(std::distance(first, last));
	size_type idx = static_cast<size_type>(first - cbegin());

	if (count == 0)
		return iterator(this, idx);

	pool_base pb = get_pool();

	transaction::run(pb, [&] {
		/*
		 * XXX: future optimization: no need to snapshot trivial types,
		 * if idx + count = _size
		 */
		snapshot_data(idx, _size);

		iterator dest = iterator(this, idx);
		iterator start = iterator(this, idx + count);
		iterator end = iterator(this, size());

		std::move(start, end, dest);

		_size -= count;
	});

	assert(segment_capacity_validation());

	return iterator(this, idx);
}

/**
 * Appends the given element value to the end of the container transactionally.
 * The new element is initialized as a copy of value.
 *
 * @param[in] value the value of the element to be appended.
 *
 * @pre value_type must meet the requirements of CopyInsertable.
 *
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity, or remains the same if there is enough space to add single element.
 *
 * @throw transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::push_back(const T &value)
{
	emplace_back(value);
}

/**
 * Appends the given element value to the end of the container transactionally.
 * value is moved into the new element.
 *
 * @param[in] value the value of the element to be appended.
 *
 * @pre value_type must meet the requirements of MoveInsertable.
 *
 * @post size() == size() + 1
 * @post capacity() is equal to the smallest next power of 2, bigger than old
 * capacity, or remains the same if there is enough space to add single element.
 *
 * @throw transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::push_back(T &&value)
{
	emplace_back(std::move(value));
}

/**
 * Removes the last element of the container transactionally. Calling pop_back
 * on an empty container does nothing. No iterators or references except for
 * back() and end() are invalidated.
 *
 * @post size() == std::max(0, size() - 1)
 *
 * @throw transaction_error when snapshotting failed.
 * @throw rethrows desctructor exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::pop_back()
{
	if (_size == 0)
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] { shrink(_size - 1); });
}

/**
 * Resizes the container to count elements transactionally. If the current size
 * is greater than count, the container is reduced to its first count elements.
 * If the current size is less than count, additional default-inserted elements
 * are appended.
 *
 * @param[in] count new size of the container
 *
 * @post capacity() == std::max(count, capacity())
 * @post size() == count
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::resize(size_type count)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (count <= _size)
			shrink(count);
		else {
			if (_capacity < count)
				internal_reserve(count);
			construct(_size, count - _size);
		}
	});
}

/**
 * Resizes the container to contain count elements transactionally. If the
 * current size is greater than count, the container is reduced to its first
 * count elements. If the current size is less than count, additional copies of
 * value are appended.
 *
 * @param[in] count new size of the container.
 * @param[in] value the value to initialize the new elements with.
 *
 * @post capacity() == count
 * @post size() == std::min(_size, count)
 *
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::resize(size_type count, const value_type &value)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (count <= _size)
			shrink(count);
		else {
			if (_capacity < count)
				internal_reserve(count);
			construct(_size, count - _size, value);
		}
	});
}

/**
 * Exchanges the contents of the container with other transactionally.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::swap(segment_table &other)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		std::swap(this->_data, other._data);
		std::swap(this->_size, other._size);
		std::swap(this->_capacity, other._capacity);
	});
}

/**
 * Private helper method. Increases capacity.
 * Allocs new segments if new_capacity's segment greater than capacity()'s
 * segment.
 *
 * @pre must be called in transaction scope
 *
 * @param[in] new_capacity new desired capacity of the container.
 *
 * @post capacity() = first power of 2 larger than new_capacity
 *
 * @throw pmem::length_error when new_capacity larger than max_size().
 * @throw pmem::transaction_out_of_memory when called out of transaction scope.
 * @throw pmem::transaction_alloc_error when there is not enough free space.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::internal_reserve(size_type new_capacity)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (new_capacity > max_size())
		throw std::length_error("New capacity exceeds max size.");

	if (new_capacity == 0)
		return;

	size_type old_idx = Policy::get_segment(_capacity);
	size_type new_idx = Policy::get_segment(new_capacity - 1);
	for (size_type i = old_idx; i <= new_idx; ++i) {
		size_type segment_capacity = Policy::segment_size(i);
		_data[i].reserve(segment_capacity);
	}

	_capacity = Policy::segment_top(new_idx + 1);

	assert(segment_capacity_validation());
}

/**
 * Private helper method. Deallocates all elements in segment_table.
 *
 * @pre must be called in transaction scope
 *
 * @post size() = capacity() = 0
 * @post data[i] = nullptr
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::dealloc()
{
	for (size_type i = 0; i <= Policy::get_segment(_capacity); ++i) {
		_data[i].~vector();
	}
	_size = _capacity = 0;
}

/**
 * Private helper function. Must be called during transaction. Assumes that
 * there is free space for additional elements. Constructs elements at given
 * index in underlying segments based on given parameters.
 *
 * @param[in] idx underlying segments index where new elements will be
 * constructed.
 * @param[in] count number of elements to be constructed.
 * @param[in] args variadic template arguments for value_type constructor.
 *
 * @pre must be called in transaction scope.
 * @pre if initialized, range [end(), end() + count) must be snapshotted in
 * current transaction.
 * @pre capacity() >= count + size()
 * @pre args is valid argument for value_type constructor.
 *
 * @post size() == size() + count
 *
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
template <typename... Args>
void
segment_table<T, Policy>::construct(size_type idx, size_type count,
				    Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(_capacity >= count + _size);

	for (size_type i = idx; i < idx + count; ++i) {
		size_type segment = Policy::get_segment(i);
		_data[segment].emplace_back(std::forward<Args>(args)...);
	}
	_size += count;
}

/**
 * Private helper function. Must be called during transaction. Assumes that
 * there is free space for additional elements and input arguments satisfy
 * InputIterator requirements. Moves elements at index idx in underlying
 * segments with the contents of the range [first, last). This overload
 * participates in overload resolution only if InputIt satisfies InputIterator.
 *
 * @param[in] idx segment_table index where new elements will be moved.
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @pre must be called in transaction scope.
 * @pre if initialized, range [end(), end() + std::distance(first, last)) must
 * be snapshotted in current transaction.
 * @pre capacity() >= std::distance(first, last) + size()
 * @pre InputIt is InputIterator.
 * @pre std::move(InputIt::reference) is valid argument for value_type
 * constructor.
 *
 * @post size() == size() + std::distance(first, last)
 *
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
void
segment_table<T, Policy>::construct_range(size_type idx, InputIt first,
					  InputIt last)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	size_type count = static_cast<size_type>(std::distance(first, last));
	assert(count >= 0);
	assert(_capacity >= count + _size);

	for (size_type i = idx; i < idx + count; ++i, ++first) {
		size_type segment = Policy::get_segment(i);
		_data[segment].emplace_back(std::move(*first));
	}
	_size += count;
}

/**
 * Private helper function. Must be called during transaction. Assumes that
 * there is free space for additional elements and input arguments satisfy
 * InputIterator requirements. Moves elements at index idx in underlying
 * segments with the contents of the range [first, last). This overload
 * participates in overload resolution only if InputIt satisfies InputIterator.
 *
 * @param[in] idx underyling segments index where new elements will be moved.
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @pre must be called in transaction scope.
 * @pre if initialized, range [end(), end() + std::distance(first, last)) must
 * be snapshotted in current transaction.
 * @pre capacity() >= std::distance(first, last) + size()
 * @pre InputIt is InputIterator.
 * @pre std::move(InputIt::reference) is valid argument for value_type
 * constructor.
 *
 * @post size() == size() + std::distance(first, last)
 *
 * @throw rethrows constructor exception.
 */
template <typename T, typename Policy>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
void
segment_table<T, Policy>::construct_range_copy(size_type idx, InputIt first,
					       InputIt last)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	size_type count = static_cast<size_type>(std::distance(first, last));
	assert(count >= 0);
	assert(_capacity >= count + _size);

	for (size_type i = idx; i < idx + count; ++i, ++first) {
		size_type segment = Policy::get_segment(i);
		_data[segment].emplace_back(*first);
	}
	_size += count;
}

/**
 * Private helper function. Must be called during transaction. Inserts a gap for
 * count elements starting at index idx. If there is not enough space available,
 * reallocation occurs.
 *
 * param[in] idx index number where gap should be made.
 * param[in] count length (expressed in number of elements) of the gap.
 *
 * @pre must be called in transaction scope.
 *
 * @post if there is not enough space for additional gap, new segment will be
 * allocated and capacity() will equal to smallest power of 2 bigger than
 * capacity().
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying segments
 * failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::insert_gap(size_type idx, size_type count)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (_capacity < _size + count)
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
 * @pre if initialized, range [begin(), end()) must be snapshotted in current
 * transaction.
 * @pre size_new <= size()
 *
 * @post size() == size_new
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::shrink(size_type size_new)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size_new <= _size);

	size_type from = Policy::get_segment(_size - 1);
	size_type to = Policy::get_segment(size_new);
	for (size_type i = from; i > to; --i)
		_data[i].clear();
	size_type local_idx = Policy::segment_local(size_new);
	_data[to].resize(local_idx);

	assert(segment_capacity_validation());

	_size = size_new;
}

/**
 * Private helper function. Checks if segment_table resides on pmem and throws
 * an exception if not.
 *
 * @throw pool_error if segment_table doesn't reside on pmem.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::check_pmem()
{
	if (nullptr == pmemobj_pool_by_ptr(this))
		throw pmem::pool_error("Invalid pool handle.");
}

/**
 * Private helper function. Checks if current transaction stage is equal to
 * TX_STAGE_WORK and throws an exception otherwise.
 *
 * @throw pmem::transaction_scope_error if current transaction stage is not
 * equal to TX_STAGE_WORK.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::check_tx_stage_work()
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"Function called out of transaction scope.");
}

/**
 * Private helper function.
 *
 * @return reference to pool_base object where segment_table resides.
 *
 * @pre underlying segments must reside in persistent memory pool.
 */
template <typename T, typename Policy>
pool_base
segment_table<T, Policy>::get_pool() const noexcept
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);
	return pool_base(pop);
}

/**
 * Private helper function. Takes a “snapshot” of data in range
 * [this[idx_first], this[idx_last])
 *
 * @param[in] idx_first first index.
 * @param[in] idx_last last index.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename T, typename Policy>
void
segment_table<T, Policy>::snapshot_data(size_type idx_first, size_type idx_last)
{
	for (size_type i = idx_first; i < idx_last; ++i) {
		detail::conditional_add_to_tx(&get(i));
	}
}

/**
 * Private helper function. Not considering if element exist or not.
 *
 * @return reference to element with given index in segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::reference
segment_table<T, Policy>::get(size_type n)
{
	size_type s_idx = Policy::get_segment(n);
	size_type local_idx = Policy::segment_local(n);

	return _data[s_idx][local_idx];
}

/**
 * Private helper function. Not considering if element exist or not.
 *
 * @return const reference to element with given index in segment_table.
 */
template <typename T, typename Policy>
typename segment_table<T, Policy>::const_reference
segment_table<T, Policy>::get(size_type n) const
{
	size_type s_idx = Policy::get_segment(n);
	size_type local_idx = Policy::segment_local(n);

	return _data[s_idx][local_idx];
}

/**
 * Private helper function. Checks if each allocated segment match its expected
 * capacity according to segment_policy
 */
template <typename T, typename Policy>
bool
segment_table<T, Policy>::segment_capacity_validation()
{
	for (size_type i = 0; i < Policy::get_segment(_capacity - 1); ++i)
		if (_data[i].capacity() != Policy::segment_size(i))
			return false;
	return true;
}

/**
 * Swaps the contents of lhs and rhs.
 *
 * @param[in] lhs first segment_table
 * @param[in] rhs second segment_table
 */
template <typename T, typename Policy>
void
swap(segment_table<T, Policy> &lhs, segment_table<T, Policy> &rhs)
{
	lhs.swap(rhs);
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each element in lhs
 * is equal to element in rhs at the same position.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of the containers are equal, false otherwise
 */
template <typename T, typename Policy>
bool
operator==(const segment_table<T, Policy> &lhs,
	   const segment_table<T, Policy> &rhs)
{
	return lhs.size() == rhs.size() &&
		std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the different number of elements or at least one
 * element in lhs is not equal to element in rhs at the same position.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of the containers are not equal, false otherwise
 */
template <typename T, typename Policy>
bool
operator!=(const segment_table<T, Policy> &lhs,
	   const segment_table<T, Policy> &rhs)
{
	return !(lhs == rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of lhs are lexicographically less than contents of
 * rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator<(const segment_table<T, Policy> &lhs,
	  const segment_table<T, Policy> &rhs)
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
					    rhs.end());
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is pmem::obj::experime ntal::segment_table<T, Policy>
 *
 * @return true if contents of lhs are lexicographically lesser than or equal to
 * contents of rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator<=(const segment_table<T, Policy> &lhs,
	   const segment_table<T, Policy> &rhs)
{
	return !(rhs < lhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of lhs are lexicographically greater than contents
 * of rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator>(const segment_table<T, Policy> &lhs,
	  const segment_table<T, Policy> &rhs)
{
	return rhs < lhs;
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of lhs are lexicographically greater than or equal
 * to contents of rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator>=(const segment_table<T, Policy> &lhs,
	   const segment_table<T, Policy> &rhs)
{
	return !(lhs < rhs);
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each element in lhs
 * is equal to element in rhs at the same position.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is std::vector<T>
 *
 * @return true if contents of the containers are equal, false otherwise
 */
template <typename T, typename Policy>
bool
operator==(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs)
{
	return lhs.size() == rhs.size() &&
		std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each element in lhs
 * is equal to element in rhs at the same position.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is std::vector<T>
 *
 * @return true if contents of the containers are not equal, false otherwise
 */
template <typename T, typename Policy>
bool
operator!=(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs)
{
	return !(lhs == rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is std::vector<T>
 *
 * @return true if contents of lhs are lexicographically less than contents of
 * rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator<(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs)
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
					    rhs.end());
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is std::vector<T>
 *
 * @return true if contents of lhs are lexicographically lesser than or equal to
 * contents of rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator<=(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs)
{
	return !(std::lexicographical_compare(rhs.begin(), rhs.end(),
					      lhs.begin(), lhs.end()));
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is std::vector<T>
 *
 * @return true if contents of lhs are lexicographically greater than contents
 * of rhs, false otherwise
 */

template <typename T, typename Policy>
bool
operator>(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs)
{
	return !(lhs <= rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is pmem::obj::experimental::segment_table<T, Policy>
 * @param[in] rhs is std::vector<T>
 *
 * @return true if contents of lhs are lexicographically greater than or equal
 * to contents of rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator>=(const segment_table<T, Policy> &lhs, const std::vector<T> &rhs)
{
	return !(lhs < rhs);
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each element in lhs
 * is equal to element in rhs at the same position.
 *
 * @param[in] lhs is std::vector<T>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of the containers are equal, false otherwise
 */
template <typename T, typename Policy>
bool
operator==(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs)
{
	return rhs == lhs;
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each element in lhs
 * is equal to element in rhs at the same position.
 *
 * @param[in] lhs is std::vector<T>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of the containers are not equal, false otherwise
 */
template <typename T, typename Policy>
bool
operator!=(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs)
{
	return !(lhs == rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is std::vector<T>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of lhs are lexicographically less than contents of
 * rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator<(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs)
{
	return rhs > lhs;
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is std::vector<T>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of lhs are lexicographically lesser than or equal to
 * contents of rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator<=(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs)
{
	return !(rhs < lhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is std::vector<T>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of lhs are lexicographically greater than contents
 * of rhs, false otherwise
 */

template <typename T, typename Policy>
bool
operator>(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs)
{
	return rhs < lhs;
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs is std::vector<T>
 * @param[in] rhs is pmem::obj::experimental::segment_table<T, Policy>
 *
 * @return true if contents of lhs are lexicographically greater than or equal
 * to contents of rhs, false otherwise
 */
template <typename T, typename Policy>
bool
operator>=(const std::vector<T> &lhs, const segment_table<T, Policy> &rhs)
{
	return !(lhs < rhs);
}

/**
 * Default constructor. Constructs an empty iterator.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>::segment_iterator() noexcept
    : table(nullptr), index()
{
}

/**
 * segment_itarator constructor. Constructs iterator with given container and
 * index to position in it.
 */
template <typename Container, bool is_const>
segment_iterator<Container, is_const>::segment_iterator(table_ptr tab,
							size_type idx) noexcept
    : table(tab), index(idx)
{
}

/** Copy constructor for const iterator from non-const iterator */
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
 * Random acess incrementing.
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
 * Random acess incrementing with assignment.
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
 * Random acess decrementing.
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
 * Random acess decrementing with assignment.
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
operator+(const segment_iterator<Container, C> &rhs)
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
operator-(const segment_iterator<Container, C> &rhs)
{
	return static_cast<difference_type>(index - rhs.index);
}

/**
 * Equality operator.
 *
 * @param[in] segment_iterator<Container, C> to compare with
 *
 * @return true if rhs is equal to current iterator, false overwise.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator==(const segment_iterator<Container, C> &rhs)
{
	return (table == rhs.table) && (index == rhs.index);
}

/**
 * Not equal operator.
 *
 * @param[in] segment_iterator<Container, C> to compare with
 *
 * @return true if rhs is not equal to current iterator, false overwise.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator!=(const segment_iterator<Container, C> &rhs)
{
	return (table != rhs.table) || (index != rhs.index);
}

/**
 * Less operator.
 *
 * @param[in] segment_iterator<Container, C> to compare with
 *
 * @return true if current index less than rhs index, false overwise.
 *
 * @throw std::invalid_argument if rhs created on other Container instance.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator<(const segment_iterator<Container, C> &rhs)
{
	if (table != rhs.table)
		throw std::invalid_argument("segment_iterator::operator<");

	return index < rhs.index;
}

/**
 * Greater operator.
 *
 * @param[in] segment_iterator<Container, C> to compare with
 *
 * @return true if current index greater than rhs index, false overwise.
 *
 * @throw std::invalid_argument if rhs created on other Container instance.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator>(const segment_iterator<Container, C> &rhs)
{
	if (table != rhs.table)
		throw std::invalid_argument("segment_iterator::operator<");

	return index > rhs.index;
}

/**
 * Less or equal operator.
 *
 * @param[in] segment_iterator<Container, C> to compare with
 *
 * @return true if current index less or equal to rhs index, false overwise.
 *
 * @throw std::invalid_argument if rhs created on other Container instance.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator<=(const segment_iterator<Container, C> &rhs)
{
	if (table != rhs.table)
		throw std::invalid_argument("segment_iterator::operator<");

	return index <= rhs.index;
}

/**
 * Greater or equal operator.
 *
 * @param[in] segment_iterator<Container, C> to compare with
 *
 * @return true if current index greater or equal to rhs index, false overwise.
 *
 * @throw std::invalid_argument if rhs created on other Container instance.
 */
template <typename Container, bool is_const>
template <bool C>
bool
segment_iterator<Container, is_const>::
operator>=(const segment_iterator<Container, C> &rhs)
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

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */

#endif /* PMEMOBJ_SEGMENT_TABLE_HPP */
