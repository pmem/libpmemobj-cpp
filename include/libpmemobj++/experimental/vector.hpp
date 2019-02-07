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
 * Vector container with std::vector compatible interface.
 */

#ifndef LIBPMEMOBJ_CPP_VECTOR_HPP
#define LIBPMEMOBJ_CPP_VECTOR_HPP

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/iterator_traits.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/experimental/contiguous_iterator.hpp>
#include <libpmemobj++/experimental/slice.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj.h>

#include <algorithm>
#include <cassert>
#include <utility>

namespace pmem
{

namespace obj
{

namespace experimental
{

/**
 * pmem::obj::experimental::vector - EXPERIMENTAL persistent container
 * with std::vector compatible interface.
 */
template <typename T>
class vector {
public:
	/* Member types */
	using value_type = T;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using iterator = basic_contiguous_iterator<T>;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	/* Constructors */
	vector();
	vector(size_type count, const value_type &value);
	explicit vector(size_type count);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value,
			  InputIt>::type * = nullptr>
	vector(InputIt first, InputIt last);
	vector(const vector &other);
	vector(vector &&other);
	vector(std::initializer_list<T> init);

	/* Assign operators */
	// vector &operator=(const vector &other);
	// vector &operator=(vector &&other);
	// vector &operator=(std::initializer_list<T> ilist);

	/* Assign methods */
	// void assign(size_type count, const T &value);
	// template <typename InputIt>
	// void assign(InputIt first, typename
	// std::enable_if<detail::is_input_iterator<InputIt>::value &&
	// std::is_constructible<value_type, typename
	// std::iterator_traits<InputIt>::reference>::value, InputIt>::type
	// last); void assign(std::initializer_list<T> ilist);

	/* Destructor */
	~vector();

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
	value_type *data();
	const value_type *data() const noexcept;
	const value_type *cdata() const noexcept;

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
	// iterator insert(const_iterator pos, const T &value);
	// iterator insert(const_iterator pos, T &&value);
	// iterator insert(const_iterator pos, size_type count, const T &value);
	// template <typename InputIt>
	// iterator insert(const_iterator pos, InputIt first, typename
	// std::enable_if<detail::is_input_iterator<InputIt>::value,
	// InputIt>::type last); iterator insert(const_iterator pos,
	// std::initializer_list<T> ilist); template <class... Args> iterator
	// emplace(const_iterator pos, Args&&... args); template< class... Args
	// > void emplace_back(Args&&... args); iterator erase(iterator pos);
	// iterator erase(const_iterator pos);
	// iterator erase(iterator first, iterator last);
	// iterator erase(const_iterator first, const_iterator last);
	// void push_back(const T& value);
	// void push_back(T&& value);
	// void pop_back();
	void resize(size_type count);
	void resize(size_type count, const value_type &value);
	void swap(vector &other);

private:
	/* helper functions */
	void alloc(size_type size);
	void check_pmem();
	void check_tx_stage_work();
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
	void dealloc();
	pool_base get_pool() const noexcept;
	void insert_gap(size_type idx, size_type count);
	void realloc(size_type size);
	size_type get_recommended_capacity(size_type at_least) const;
	void shrink(size_type size_new);
	void snapshot_data(size_type count = 0);

	/* Underlying array */
	persistent_ptr<T[]> _data;

	p<size_type> _size;
	p<size_type> _capacity;
};

/* Comparison operators */
template <typename T>
bool operator==(const vector<T> &lhs, const vector<T> &rhs);
template <typename T>
bool operator!=(const vector<T> &lhs, const vector<T> &rhs);
template <typename T>
bool operator<(const vector<T> &lhs, const vector<T> &rhs);
template <typename T>
bool operator<=(const vector<T> &lhs, const vector<T> &rhs);
template <typename T>
bool operator>(const vector<T> &lhs, const vector<T> &rhs);
template <typename T>
bool operator>=(const vector<T> &lhs, const vector<T> &rhs);

/**
 * Default constructor. Constructs an empty container.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 */
template <typename T>
vector<T>::vector()
{
	check_pmem();
	check_tx_stage_work();

	_data = nullptr;
	_size = 0;
	_capacity = 0;
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
 * @throw pmem::transaction_alloc_error when allocating memory for underlying
 * array in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 */
template <typename T>
vector<T>::vector(size_type count, const value_type &value)
{
	check_pmem();
	check_tx_stage_work();

	_data = nullptr;
	_size = 0;
	alloc(count);
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
 * @throw pmem::transaction_alloc_error when allocating memory for underlying
 * array in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 */
template <typename T>
vector<T>::vector(size_type count)
{
	check_pmem();
	check_tx_stage_work();

	_data = nullptr;
	_size = 0;
	alloc(count);
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
 * @throw pmem::transaction_alloc_error when allocating memory for underlying
 * array in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 */
template <typename T>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
vector<T>::vector(InputIt first, InputIt last)
{
	check_pmem();
	check_tx_stage_work();

	_data = nullptr;
	_size = 0;
	alloc(static_cast<size_type>(std::distance(first, last)));
	construct_range_copy(0, first, last);
}

/**
 * Copy constructor. Constructs the container with the copy of the
 * contents of other.
 *
 * @param[in] other reference to the vector to be copied.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for underlying
 * array in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 */
template <typename T>
vector<T>::vector(const vector &other)
{
	check_pmem();
	check_tx_stage_work();

	_data = nullptr;
	_size = 0;
	alloc(other.capacity());
	construct_range_copy(0, other.begin(), other.end());
}

/**
 * Move constructor. Constructs the container with the contents of other using
 * move semantics. After the move, other is guaranteed to be empty().
 *
 * @param[in] other rvalue reference to the vector to be moved from.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == other.size()
 * @post capacity() == other.capacity()
 * @post data() == other.data()
 * @post other.data() == nullptr
 * @post other.capacity() == 0
 * @post other.size() == 0
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 */
template <typename T>
vector<T>::vector(vector &&other)
{
	check_pmem();
	check_tx_stage_work();

	_data = other._data;
	_capacity = other.capacity();
	_size = other.size();
	other._data = nullptr;
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
 * @throw pmem::transaction_alloc_error when allocating memory for underlying
 * array in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in transaction.
 * @throw rethrows element constructor exception.
 */
template <typename T>
vector<T>::vector(std::initializer_list<T> init)
    : vector(init.begin(), init.end())
{
}

/**
 * Destructor.
 * Note that free_data may throw an transaction_free_error when freeing
 * underlying array failed. It is recommended to call free_data manually before
 * object destruction.
 *
 * @throw rethrows destructor exception.
 * @throw transaction_free_error when freeing underlying array failed.
 */
template <typename T>
vector<T>::~vector()
{
	free_data();
}

/**
 * Access element at specific index with bounds checking and add it to a
 * transaction.
 *
 * @param[in] n index number.
 *
 * @return reference to element number n in underlying array.
 *
 * @pre must be called in transaction scope.
 *
 * @throw std::out_of_range if n is not within the range of the container.
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T>
typename vector<T>::reference
vector<T>::at(size_type n)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	if (n >= _size)
		throw std::out_of_range("vector::at");
	detail::conditional_add_to_tx(&_data[n]);
	return _data[n];
}

/**
 * Access element at specific index with bounds checking.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying array.
 *
 * @throw std::out_of_range if n is not within the range of the container.
 */
template <typename T>
typename vector<T>::const_reference
vector<T>::at(size_type n) const
{
	if (n >= _size)
		throw std::out_of_range("vector::at");
	return _data[n];
}

/**
 * Access element at specific index with bounds checking. In contradiction to
 * at(), const_at() will return const_reference not depending on the
 * const-qualification of the object it is called on. std::vector doesn't
 * provide const_at() method.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying array.
 *
 * @throw std::out_of_range if n is not within the range of the container.
 */
template <typename T>
typename vector<T>::const_reference
vector<T>::const_at(size_type n) const
{
	if (n >= _size)
		throw std::out_of_range("vector::const_at");
	return _data[n];
}

/**
 * Access element at specific index and add it to a transaction. No bounds
 * checking is performed.
 *
 * @param[in] n index number.
 *
 * @return reference to element number n in underlying array.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T>
typename vector<T>::reference vector<T>::operator[](size_type n)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	detail::conditional_add_to_tx(&_data[n]);
	return _data[n];
}

/**
 * Access element at specific index. No bounds checking is performed.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying array.
 */
template <typename T>
typename vector<T>::const_reference vector<T>::operator[](size_type n) const
{
	return _data[n];
}

/**
 * Access the first element and add this element to a transaction.
 *
 * @return reference to first element in underlying array.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T>
typename vector<T>::reference
vector<T>::front()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	detail::conditional_add_to_tx(&_data[0]);
	return _data[0];
}

/**
 * Access the first element.
 *
 * @return const_reference to first element in underlying array.
 */
template <typename T>
typename vector<T>::const_reference
vector<T>::front() const
{
	return _data[0];
}

/**
 * Access the first element. In contradiction to front(), cfront() will return
 * const_reference not depending on the const-qualification of the object it is
 * called on. std::vector doesn't provide cfront() method.
 *
 * @return reference to first element in underlying array.
 */
template <typename T>
typename vector<T>::const_reference
vector<T>::cfront() const
{
	return _data[0];
}

/**
 * Access the last element and add this element to a transaction.
 *
 * @return reference to the last element in underlying array.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T>
typename vector<T>::reference
vector<T>::back()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	detail::conditional_add_to_tx(&_data[size() - 1]);
	return _data[size() - 1];
}

/**
 * Access the last element.
 *
 * @return const_reference to the last element in underlying array.
 */
template <typename T>
typename vector<T>::const_reference
vector<T>::back() const
{
	return _data[size() - 1];
}

/**
 * Access the last element. In contradiction to back(), cback() will return
 * const_reference not depending on the const-qualification of the object it is
 * called on. std::vector doesn't provide cback() method.
 *
 * @return const_reference to the last element in underlying array.
 */
template <typename T>
typename vector<T>::const_reference
vector<T>::cback() const
{
	return _data[size() - 1];
}

/**
 * Returns raw pointer to the underlying data and adds entire array to a
 * transaction.
 *
 * @return pointer to the underlying data.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::transaction_error when adding the object to the transaction
 * failed.
 */
template <typename T>
typename vector<T>::value_type *
vector<T>::data()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	snapshot_data();
	return _data.get();
}

/**
 * Returns const raw pointer to the underlying data.
 *
 * @return const_pointer to the underlying data.
 */
template <typename T>
const typename vector<T>::value_type *
vector<T>::data() const noexcept
{
	return _data.get();
}

/**
 * Returns const raw pointer to the underlying data. In contradiction to data(),
 * cdata() will return const_pointer not depending on the const-qualification of
 * the object it is called on. std::vector doesn't provide cdata() method.
 *
 * @return const_pointer to the underlying data.
 */
template <typename T>
const typename vector<T>::value_type *
vector<T>::cdata() const noexcept
{
	return _data.get();
}

/**
 * Returns an iterator to the beginning.
 *
 * @return iterator pointing to the first element in the vector.
 *
 * @pre must be called in transaction scope.
 */
template <typename T>
typename vector<T>::iterator
vector<T>::begin()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	return iterator(_data.get());
}

/**
 * Returns const iterator to the beginning.
 *
 * @return const_iterator pointing to the first element in the vector.
 */
template <typename T>
typename vector<T>::const_iterator
vector<T>::begin() const noexcept
{
	return const_iterator(_data.get());
}

/**
 * Returns const iterator to the beginning. In contradiction to begin(),
 * cbegin() will return const_iterator not depending on the const-qualification
 * of the object it is called on.
 *
 * @return const_iterator pointing to the first element in the vector.
 */
template <typename T>
typename vector<T>::const_iterator
vector<T>::cbegin() const noexcept
{
	return const_iterator(_data.get());
}

/**
 * Returns an iterator to past the end.
 *
 * @return iterator referring to the past-the-end element in the vector.
 *
 * @pre must be called in transaction scope.
 */
template <typename T>
typename vector<T>::iterator
vector<T>::end()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	return iterator(_data.get() + static_cast<std::ptrdiff_t>(_size));
}

/**
 * Returns a const iterator to past the end.
 *
 * @return const_iterator referring to the past-the-end element in the vector.
 */
template <typename T>
typename vector<T>::const_iterator
vector<T>::end() const noexcept
{
	return const_iterator(_data.get() + static_cast<std::ptrdiff_t>(_size));
}

/**
 * Returns a const iterator to the end. In contradiction to end(), cend() will
 * return const_iterator not depending on the const-qualification of the object
 * it is called on.
 *
 * @return const_iterator referring to the past-the-end element in the vector.
 */
template <typename T>
typename vector<T>::const_iterator
vector<T>::cend() const noexcept
{
	return const_iterator(_data.get() + static_cast<std::ptrdiff_t>(_size));
}

/**
 * Returns a reverse iterator to the beginning.
 *
 * @return reverse_iterator pointing to the last element in the vector.
 *
 * @pre must be called in transaction scope.
 */
template <typename T>
typename vector<T>::reverse_iterator
vector<T>::rbegin()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	return reverse_iterator(end());
}

/**
 * Returns a const reverse iterator to the beginning.
 *
 * @return const_reverse_iterator pointing to the last element in the vector.
 */
template <typename T>
typename vector<T>::const_reverse_iterator
vector<T>::rbegin() const noexcept
{
	return const_reverse_iterator(cend());
}

/**
 * Returns a const reverse iterator to the beginning. In contradiction to
 * rbegin(), crbegin() will return const_reverse_iterator not depending on the
 * const-qualification of the object it is called on.
 *
 * @return const_reverse_iterator pointing to the last element in the vector.
 */
template <typename T>
typename vector<T>::const_reverse_iterator
vector<T>::crbegin() const noexcept
{
	return const_reverse_iterator(cend());
}

/**
 * Returns a reverse iterator to the end.
 *
 * @return reverse_iterator pointing to the theoretical element preceding the
 * first element in the vector.
 *
 * @pre must be called in transaction scope.
 */
template <typename T>
typename vector<T>::reverse_iterator
vector<T>::rend()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	return reverse_iterator(begin());
}

/**
 * Returns a const reverse iterator to the end.
 *
 * @return const_reverse_iterator pointing to the theoretical element preceding
 * the first element in the vector.
 */
template <typename T>
typename vector<T>::const_reverse_iterator
vector<T>::rend() const noexcept
{
	return const_reverse_iterator(cbegin());
}

/**
 * Returns a const reverse iterator to the beginning. In contradiction to
 * rend(), crend() will return const_reverse_iterator not depending on the
 * const-qualification of the object it is called on.
 *
 * @return const_reverse_iterator pointing to the theoretical element preceding
 * the first element in the vector.
 */
template <typename T>
typename vector<T>::const_reverse_iterator
vector<T>::crend() const noexcept
{
	return const_reverse_iterator(cbegin());
}

/**
 * Checks whether the container is empty.
 *
 * @return true if container is empty, false otherwise.
 */
template <typename T>
constexpr bool
vector<T>::empty() const noexcept
{
	return _size == 0;
}

/**
 * @return number of elements.
 */
template <typename T>
typename vector<T>::size_type
vector<T>::size() const noexcept
{
	return _size;
}

/**
 * @return maximum number of elements the container is able to hold due to PMDK
 * limitations.
 */
template <typename T>
constexpr typename vector<T>::size_type
vector<T>::max_size() const noexcept
{
	return PMEMOBJ_MAX_ALLOC_SIZE / sizeof(value_type);
}

/**
 * Increases the capacity of the vector to capacity_new transactionally. If
 * capacity_new is greater than the current capacity(), new storage is
 * allocated, otherwise the method does nothing. If capacity_new is greater than
 * capacity(), all iterators, including the past-the-end iterator, and all
 * references to the elements are invalidated. Otherwise, no iterators or
 * references are invalidated.
 *
 * @param[in] capacity_new new capacity.
 *
 * @post capacity() == max(capacity(), capacity_new)
 *
 * @throw rethrows destructor exception.
 * @throw std::length_error if new_cap > max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 */
template <typename T>
void
vector<T>::reserve(size_type capacity_new)
{
	if (capacity_new <= _capacity)
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] { realloc(capacity_new); });
}

/**
 * @return number of elements that can be held in currently allocated storage
 */
template <typename T>
typename vector<T>::size_type
vector<T>::capacity() const noexcept
{
	return _capacity;
}

/**
 * Requests transactional removal of unused capacity. New capacity will be set
 * to current vector size. If reallocation occurs, all iterators, including the
 * past the end iterator, and all references to the elements are invalidated.
 * If no reallocation takes place, no iterators or references are invalidated.
 *
 * @post capacity() == size()
 *
 * @throw pmem::transaction_error when snapshotting failed
 * @throw pmem::transaction_alloc_error when reallocating failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 */
template <typename T>
void
vector<T>::shrink_to_fit()
{
	size_type capacity_new = size();
	if (capacity() == capacity_new)
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] { realloc(capacity_new); });
}

/**
 * Clears the content of a vector transactionally.
 *
 * @post size() == 0
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename T>
void
vector<T>::clear()
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] { shrink(0); });
}

/**
 * Clears the content of a vector and frees all allocated persistent memory for
 * data transactionally.
 *
 * @post size() == 0
 * @post capacity() == 0
 * @post data() == nullptr
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing underlying array failed.
 */
template <typename T>
void
vector<T>::free_data()
{
	if (_data == nullptr)
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] { dealloc(); });
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
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 */
template <typename T>
void
vector<T>::resize(size_type count)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (count <= _size)
			shrink(count);
		else {
			if (_capacity < count)
				realloc(count);
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
template <typename T>
void
vector<T>::resize(size_type count, const value_type &value)
{
	if (_capacity == count)
		return;

	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		if (count <= _size)
			shrink(count);
		else {
			if (_capacity < count)
				realloc(count);
			construct(_size, count - _size, value);
		}
	});
}

/**
 * Exchanges the contents of the container with other transactionally.
 */
template <typename T>
void
vector<T>::swap(vector &other)
{
	pool_base pb = get_pool();
	transaction::run(pb, [&] {
		std::swap(this->_data, other._data);
		std::swap(this->_size, other._size);
		std::swap(this->_capacity, other._capacity);
	});
}

/**
 * Private helper function. Must be called during transaction. Allocates memory
 * for given number of elements.
 *
 * @param[in] capacity_new capacity of new underlying array.
 *
 * @pre must be called in transaction scope.
 * @pre data() == nullptr
 * @pre size() == 0
 *
 * @post capacity() == capacity_new
 *
 * @throw std::length_error if new size exceeds biggest possible pmem
 * allocation.
 * @throw pmem::transaction_alloc_error when allocating memory for underlying
 * array in transaction failed.
 */
template <typename T>
void
vector<T>::alloc(size_type capacity_new)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(_data == nullptr);
	assert(_size == 0);

	if (capacity_new > max_size())
		throw std::length_error("New capacity exceeds max size.");

	_capacity = capacity_new;

	if (capacity_new == 0)
		return;

	/*
	 * We need to cache pmemobj_tx_alloc return value and only after that
	 * assign it to _data, because when pmemobj_tx_alloc fails, it aborts
	 * transaction.
	 */
	persistent_ptr<T[]> res =
		pmemobj_tx_alloc(sizeof(value_type) * capacity_new,
				 detail::type_num<value_type>());

	if (res == nullptr)
		throw transaction_alloc_error(
			"Failed to allocate persistent memory object");

	_data = res;
}

/**
 * Private helper function. Checks if vector resides on pmem and throws an
 * exception if not.
 *
 * @throw pool_error if vector doesn't reside on pmem.
 */
template <typename T>
void
vector<T>::check_pmem()
{
	if (nullptr == pmemobj_pool_by_ptr(this))
		throw pool_error("Invalid pool handle.");
}

/**
 * Private helper function. Checks if current transaction stage is equal to
 * TX_STAGE_WORK and throws an exception otherwise.
 *
 * @throw pmem::transaction_error if current transaction stage is not equal to
 * TX_STAGE_WORK.
 */
template <typename T>
void
vector<T>::check_tx_stage_work()
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw transaction_error(
			"Function called out of transaction scope.");
}

/**
 * Private helper function. Must be called during transaction. Assumes that
 * there is free space for additional elements. Constructs elements at given
 * index in underlying array based on given parameters.
 *
 * @param[in] idx underyling array index where new elements will be constructed.
 * @param[in] count number of elements to construct.
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
template <typename T>
template <typename... Args>
void
vector<T>::construct(size_type idx, size_type count, Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(_capacity >= count + _size);

	pointer dest = _data.get() + idx;
	const_pointer end = dest + count;
	for (; dest != end; ++dest)
		detail::create<value_type, Args...>(
			dest, std::forward<Args>(args)...);
	_size += count;
}

/**
 * Private helper function. Must be called during transaction. Assumes that
 * there is free space for additional elements and input arguments satisfy
 * InputIterator requirements. Moves elements at index idx in underlying array
 * with the contents of the range [first, last). This overload participates in
 * overload resolution only if InputIt satisfies InputIterator.
 *
 * @param[in] idx underyling array index where new elements will be moved.
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
template <typename T>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
void
vector<T>::construct_range(size_type idx, InputIt first, InputIt last)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	size_type range_size =
		static_cast<size_type>(std::distance(first, last));
	assert(range_size >= 0);
	assert(_capacity >= range_size + _size);

	pointer dest = _data.get() + idx;
	_size += range_size;
	while (first != last)
		detail::create<value_type>(dest++, std::move(*first++));
}

/**
 * Private helper function. Must be called during transaction. Assumes that
 * there is free space for additional elements and input arguments satisfy
 * InputIterator requirements. Copy-constructs elements before pos in underlying
 * array with the contents of the range [first, last).
 *
 * @param[in] idx underyling array index where new elements will be constructed.
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @pre must be called in transaction scope.
 * @pre if initialized, range [end(), end() + std::distance(first, last)) must
 * be snapshotted in current transaction.
 * @pre capacity() >= std::distance(first, last) + size()
 * @pre InputIt is InputIterator.
 * @pre InputIt::reference is valid argument for value_type copy constructor.
 *
 * @post size() == size() + std::distance(first, last)
 *
 * @throw rethrows constructor exception.
 */
template <typename T>
template <typename InputIt,
	  typename std::enable_if<detail::is_input_iterator<InputIt>::value,
				  InputIt>::type *>
void
vector<T>::construct_range_copy(size_type idx, InputIt first, InputIt last)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	size_type diff = static_cast<size_type>(std::distance(first, last));
	assert(diff >= 0);
	assert(_capacity >= diff + _size);

	pointer dest = _data.get() + idx;
	_size += diff;
	while (first != last)
		detail::create<value_type>(dest++, *first++);
}

/**
 * Private helper function. Must be called during transaction. Deallocates
 * underlying array.
 *
 * @pre must be called in transaction scope.
 *
 * @post size() == 0
 * @post capacity() == 0
 * @post data() == nullptr
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying array
 * failed.
 */
template <typename T>
void
vector<T>::dealloc()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (_data != nullptr) {
		shrink(0);
		if (pmemobj_tx_free(*_data.raw_ptr()) != 0)
			throw transaction_free_error(
				"failed to delete persistent memory object");
		_data = nullptr;
		_capacity = 0;
	}
}

/**
 * Private helper function.
 *
 * @return reference to pool_base object where vector resides.
 *
 * @pre underlying array must reside in persistent memory pool.
 */
template <typename T>
pool_base
vector<T>::get_pool() const noexcept
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);
	return pool_base(pop);
}

/**
 * Private helper function. Must be called during transaction. Inserts a gap for
 * count elements starting at index idx. If there is not enough space available,
 * reallocation occurs with new recommended size.
 *
 * param[in] idx index number where gap should be made.
 * param[in] count length (expressed in number of elements) of the gap.
 *
 * @pre must be called in transaction scope.
 *
 * @post if there is not enough space for additional gap, capacity changes to
 * get_recommended_capacity().
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 */
template <typename T>
void
vector<T>::insert_gap(size_type idx, size_type count)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (_capacity >= _size + count) {
		pointer dest = &_data[size() + count];
		pointer begin = &_data[idx];
		pointer end = &_data[size()];

		/*
		 * XXX: There is no necessity to snapshot uninitialized data, so
		 * we can optimize it by calling:
		 * transaction::snapshot<T>(begin, size() - idx).
		 * However, we need libpmemobj support for that, because right
		 * now pmemcheck will report an error (uninitialized part of
		 * data not added to tx).
		 *
		 * XXX: future optimization: we don't have to snapshot data
		 * which we will not overwrite
		 */
#if LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED
		VALGRIND_MAKE_MEM_DEFINED(end, sizeof(T) * count);
#endif
		transaction::snapshot<T>(begin, size() + count);

		std::move_backward(begin, end, dest);
	} else {
		/*
		 * XXX: future optimization: we don't have to snapshot data
		 * which we will not overwrite
		 */
		snapshot_data();

		auto old_data = _data;
		auto old_size = _size;
		pointer old_begin = &_data[0];
		pointer old_mid = &_data[idx];
		pointer old_end = &_data[size()];

		_data = nullptr;
		_size = _capacity = 0;

		alloc(get_recommended_capacity(old_size + count));

		construct_range(0, old_begin, old_mid);
		construct_range(idx + count, old_mid, old_end);

		/* destroy and free old data */
		for (size_type i = 0; i < old_size; ++i)
			detail::destroy<value_type>(old_data[i]);
		if (pmemobj_tx_free(old_data.raw()) != 0)
			throw transaction_free_error(
				"failed to delete persistent memory object");
	}
}

/**
 * Private helper function. Must be called during transaction. Allocates new
 * memory for capacity_new number of elements and copies or moves old elements
 * to new memory area. If the current size is greater than capacity_new, the
 * container is reduced to its first capacity_new elements.
 *
 * param[in] capacity_new new capacity.
 *
 * @pre must be called in transaction scope.
 *
 * @post capacity() == capacity_new
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 */
template <typename T>
void
vector<T>::realloc(size_type capacity_new)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	/*
	 * XXX: future optimization: we don't have to snapshot data
	 * which we will not overwrite
	 */
	snapshot_data();

	auto old_data = _data;
	auto old_size = _size;
	pointer old_begin = &_data[0];
	pointer old_end =
		capacity_new < _size ? &_data[capacity_new] : &_data[size()];

	_data = nullptr;
	_size = _capacity = 0;

	alloc(capacity_new);

	construct_range(0, old_begin, old_end);

	/* destroy and free old data */
	for (size_type i = 0; i < old_size; ++i)
		detail::destroy<value_type>(old_data[i]);
	if (pmemobj_tx_free(old_data.raw()) != 0)
		throw transaction_free_error(
			"failed to delete persistent memory object");
}

/**
 * Private helper function. Returns recommended capacity for at least at_least
 * elements.
 *
 * @return recommended new capacity.
 */
template <typename T>
typename vector<T>::size_type
vector<T>::get_recommended_capacity(size_type at_least) const
{
	return detail::next_pow_2(at_least);
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
template <typename T>
void
vector<T>::shrink(size_type size_new)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size_new <= _size);

	snapshot_data(size_new);

	for (size_type i = size_new; i < _size; ++i)
		detail::destroy<value_type>(_data[i]);
	_size = size_new;
}

/**
 * Private helper function. Takes a “snapshot” of all stored data in underlying
 * array, except first count elements.
 *
 * @param[in] count number of first elements in underlying array, which should
 * not be snapshotted.
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename T>
void
vector<T>::snapshot_data(size_type count)
{
	detail::conditional_add_to_tx(_data.get() + count, _size - count);
}

/**
 * Comparison operator. Compares the contents of two containers.
 *
 * Checks if containers have the same number of elements and each element in lhs
 * is equal to element in rhs at the same position.
 *
 * @param[in] lhs first vector
 * @param[in] rhs second vector
 *
 * @return true if contents of the containers are equal, false otherwise
 */
template <typename T>
bool
operator==(const vector<T> &lhs, const vector<T> &rhs)
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
 * @param[in] lhs first vector
 * @param[in] rhs second vector
 *
 * @return true if contents of the containers are not equal, false otherwise
 */
template <typename T>
bool
operator!=(const vector<T> &lhs, const vector<T> &rhs)
{
	return !(lhs == rhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs first vector
 * @param[in] rhs second vector
 *
 * @return true if contents of lhs are lexicographically less than contents of
 * rhs, false otherwise
 */
template <typename T>
bool
operator<(const vector<T> &lhs, const vector<T> &rhs)
{
	return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(),
					    rhs.end());
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs first vector
 * @param[in] rhs second vector
 *
 * @return true if contents of lhs are lexicographically lesser than or equal to
 * contents of rhs, false otherwise
 */
template <typename T>
bool
operator<=(const vector<T> &lhs, const vector<T> &rhs)
{
	return !(rhs < lhs);
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs first vector
 * @param[in] rhs second vector
 *
 * @return true if contents of lhs are lexicographically greater than contents
 * of rhs, false otherwise
 */

template <typename T>
bool
operator>(const vector<T> &lhs, const vector<T> &rhs)
{
	return rhs < lhs;
}

/**
 * Comparison operator. Compares the contents of two containers
 * lexicographically.
 *
 * @param[in] lhs first vector
 * @param[in] rhs second vector
 *
 * @return true if contents of lhs are lexicographically greater than or equal
 * to contents of rhs, false otherwise
 */
template <typename T>
bool
operator>=(const vector<T> &lhs, const vector<T> &rhs)
{
	return !(lhs < rhs);
}

} /* namespace experimental */

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_VECTOR_HPP */
