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
	using const_iterator = const_contiguous_iterator<T>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	/* Constructors */
	vector();
	// vector(size_type count, const value_type &value = T());
	// template <typename InputIt>
	// vector(InputIt first, typename
	// std::enable_if<detail::is_input_iterator<InputIt>::value &&
	// std::is_constructible<value_type, typename
	// std::iterator_traits<InputIt>::reference>::value, InputIt>::type
	// last); vector(const vector &other); vector(vector &&other);
	// vector(std::initializer_list<T> init);

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
	// ~vector();

	/* Element access */
	// reference at(size_type n);
	// const_reference at(size_type n) const;
	// reference operator[](size_type n);
	// const_reference operator[](size_type n) const;
	// reference front();
	// const_reference front() const;
	// reference back();
	// const_reference back() const;
	// pointer data();
	// const_pointer data() const noexcept;

	/* Iterators */
	// iterator begin();
	// const_iterator begin() const noexcept;
	// const_iterator cbegin() const noexcept;
	// iterator end();
	// const_iterator end() const noexcept;
	// const_iterator cend() const noexcept;
	// reverse_iterator rbegin();
	// const_reverse_iterator rbegin() const noexcept;
	// const_reverse_iterator crbegin() const noexcept;
	// reverse_iterator rend();
	// const_reverse_iterator rend() const noexcept;
	// const_reverse_iterator crend() const noexcept;

	/* Capacity */
	constexpr bool empty() const noexcept;
	// size_type size() const noexcept;
	constexpr size_type max_size() const noexcept;
	// void reserve(size_type capacity_new);
	// size_type capacity() const noexcept;
	// void shrink_to_fit();

	/* Modifiers */
	// void clear() noexcept;
	// void free_data();
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
	// void resize(size_type count, T value = T());
	// void resize(size_type count);
	// void resize(size_type count, const value_type& value);
	// void swap(vector &other);

private:
	/* helper functions */
	void _alloc(size_type size);
	void _dealloc();
	void _grow(size_type count, const_reference value);
	template <typename InputIt,
		  typename std::enable_if<
			  detail::is_input_iterator<InputIt>::value &&
				  std::is_constructible<
					  value_type,
					  typename std::iterator_traits<
						  InputIt>::reference>::value,
			  InputIt>::type * = nullptr>
	void _grow(InputIt first, InputIt last);
	void _shrink(size_type size_new) noexcept;

	/* Underlying array */
	persistent_ptr<T[]> _data;

	p<size_type> _size;
	p<size_type> _capacity;
};

/**
 * Default constructor. Constructs an empty container.
 *
 * @pre pmemobj_tx_stage() == TX_STAGE_WORK
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error if function wasn't called in transaction.
 */
template <typename T>
vector<T>::vector()
{
	auto pop = pmemobj_pool_by_ptr(this);
	if (pop == nullptr)
		throw pool_error("Invalid pool handle.");

	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw transaction_error(
			"Default constructor called out of transaction scope.");

	_data = nullptr;
	_size = 0;
	_capacity = 0;
}

/**
 * Checks whether the container is empty.
 *
 * @return true if container is empty, 0 otherwise.
 */
template <typename T>
constexpr bool
vector<T>::empty() const noexcept
{
	return _size == 0;
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
 * Private helper function. Must be called during transaction. Allocates memory
 * for given number of elements.
 *
 * @param[in] capacity_new capacity of new underlying array
 *
 * @throw std::length_error if new size exceeds biggest possible pmem
 * allocation.
 * @throw pmem::transaction_alloc_error when allocating memory for underlying
 * array in transaction failed.
 *
 * @pre pmemobj_tx_stage() == TX_STAGE_WORK
 * @pre _data == nullptr;
 * @pre _size == 0
 *
 * @post _capacity = capacity_new
 */
template <typename T>
void
vector<T>::_alloc(size_type capacity_new)
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
 * Private helper function. Must be called during transaction. Deallocates
 * underlying array.
 *
 * @throw pmem::transaction_free_error when freeing old underlying array
 * failed.
 *
 * @pre pmemobj_tx_stage() == TX_STAGE_WORK
 *
 * @post _size = _capacity = 0
 * @post _data = nullptr
 */
template <typename T>
void
vector<T>::_dealloc()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (_data != nullptr) {
		_shrink(0);
		if (pmemobj_tx_free(*_data.raw_ptr()) != 0)
			throw transaction_free_error(
				"failed to delete persistent memory object");
		_data = nullptr;
		_capacity = 0;
	}
}

/**
 * Private helper function. Must be called during transaction. Assumes that
 * there is enough space for additional elements. Copy constructs elements at
 * the end of underlying array based on given parameters.
 *
 * @param[in] count number of elements to construct.
 * @param[in] value value of all constructed elements.
 *
 * @throw rethrows constructor exception.
 *
 * @pre pmemobj_tx_stage() == TX_STAGE_WORK
 * @pre if range [end(), end() + count) wasn't allocated in current transaction,
 * it must be snapshotted
 * @pre _capacity >= count + _size
 * @pre value is valid argument for value_type copy constructor
 *
 * @post _size = _size + count
 */
template <typename T>
void
vector<T>::_grow(size_type count, const_reference value)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(_capacity >= count + _size);

	pointer dest = _data.get() + static_cast<size_type>(_size);
	const_pointer end = dest + count;
	for (; dest != end; ++dest)
		detail::create<value_type, const_reference>(dest, value);
	_size += count;
}

/**
 * Private helper function. Must be called during transaction. Assumes that
 * there is enough space for additional elements and input arguments satisfy
 * InputIterator requirements. Constructs elements in underlying array with the
 * contents of the range [first, last). The first and last arguments must
 * satisfy InputIterator requirements. This overload participates in overload
 * resolution only if InputIt satisfies InputIterator.
 *
 * @param[in] first first iterator.
 * @param[in] last last iterator.
 *
 * @throw rethrows constructor exception.
 *
 * @pre pmemobj_tx_stage() == TX_STAGE_WORK
 * @pre if range [end(), end() + std::distance(first, last)) wasn't allocated
 * in current transaction, it must be snapshotted
 * @pre _capacity >= std::distance(first, last) + _size
 * @pre InputIt is InputIterator
 * @pre InputIt::reference is valid argument for value_type copy constructor
 *
 * @post _size = _size + std::distance(first, last)
 */
template <typename T>
template <typename InputIt,
	  typename std::enable_if<
		  detail::is_input_iterator<InputIt>::value &&
			  std::is_constructible<
				  T,
				  typename std::iterator_traits<
					  InputIt>::reference>::value,
		  InputIt>::type *>
void
vector<T>::_grow(InputIt first, InputIt last)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	difference_type diff = std::distance(first, last);
	assert(diff >= 0);
	assert(_capacity >= static_cast<size_type>(diff) + _size);

	pointer dest = _data.get() + static_cast<size_type>(_size);
	_size += static_cast<size_type>(diff);
	while (first != last)
		detail::create<value_type>(dest++, *first++);
}

/**
 * Private helper function. Must be called during transaction. Destroys
 * elements in underlying array beginning from position size_new.
 *
 * @param[in] size_new new size
 *
 * @pre pmemobj_tx_stage() == TX_STAGE_WORK
 * @pre if range [begin(), end()) wasn't allocated in current transaction, it
 * must be snapshotted
 * transaction
 * @pre size_new <= _size
 *
 * @post _size = size_new
 */
template <typename T>
void
vector<T>::_shrink(size_type size_new) noexcept
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size_new <= _size);

	for (size_type i = size_new; i < _size; ++i)
		detail::destroy<value_type>(_data[i]);
	_size = size_new;
}

} /* namespace experimental */

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_VECTOR_HPP */
