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
 * String container with std::basic_string compatible interface.
 */

#ifndef LIBPMEMOBJ_CPP_BASIC_STRING_HPP
#define LIBPMEMOBJ_CPP_BASIC_STRING_HPP

#include <algorithm>
#include <limits>
#include <string>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/iterator_traits.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/experimental/contiguous_iterator.hpp>
#include <libpmemobj++/experimental/slice.hpp>
#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem
{

namespace obj
{

namespace experimental
{

/**
 * pmem::obj::experimental::string - EXPERIMENTAL persistent container
 * with std::basic_string compatible interface.
 *
 * The implementation is NOT complete.
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_string {
public:
	/* Member types */
	using traits_type = Traits;
	using value_type = CharT;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using iterator = basic_contiguous_iterator<CharT>;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	/* Number of characters which can be stored using sso */
	static constexpr size_type sso_capacity = 64 - sizeof('\0');

	/* Constructors */
	basic_string();
	basic_string(size_type count, CharT ch);
	basic_string(const basic_string &other, size_type pos,
		     size_type count = npos);
	basic_string(const std::basic_string<CharT> &other, size_type pos,
		     size_type count = npos);
	basic_string(const CharT *s, size_type count);
	basic_string(const CharT *s);
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	basic_string(InputIt first, InputIt last);
	basic_string(const basic_string &other);
	basic_string(const std::basic_string<CharT> &other);
	basic_string(basic_string &&other);
	basic_string(std::initializer_list<CharT> ilist);

	/* Destructor */
	~basic_string();

	/* Assignment operators */
	basic_string &operator=(const basic_string &other);
	basic_string &operator=(const std::basic_string<CharT> &other);
	basic_string &operator=(basic_string &&other);
	basic_string &operator=(const CharT *s);
	basic_string &operator=(CharT ch);
	basic_string &operator=(std::initializer_list<CharT> ilist);

	/* Assignment methods */
	basic_string &assign(size_type count, CharT ch);
	basic_string &assign(const basic_string &other);
	basic_string &assign(const std::basic_string<CharT> &other);
	basic_string &assign(const basic_string &other, size_type pos,
			     size_type count = npos);
	basic_string &assign(const std::basic_string<CharT> &other,
			     size_type pos, size_type count = npos);
	basic_string &assign(const CharT *s, size_type count);
	basic_string &assign(const CharT *s);
	template <typename InputIt,
		  typename Enable = typename pmem::detail::is_input_iterator<
			  InputIt>::type>
	basic_string &assign(InputIt first, InputIt last);
	basic_string &assign(basic_string &&other);
	basic_string &assign(std::initializer_list<CharT> ilist);

	/* Element access */
	reference at(size_type n);
	const_reference at(size_type n) const;
	const_reference const_at(size_type n) const;
	reference operator[](size_type n);
	const_reference operator[](size_type n) const;
	CharT &front();
	const CharT &front() const;
	const CharT &cfront() const;
	CharT &back();
	const CharT &back() const;
	const CharT &cback() const;
	CharT *data();
	const CharT *data() const noexcept;
	const CharT *cdata() const noexcept;
	const CharT *c_str() const noexcept;

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
	bool empty() const noexcept;
	size_type size() const noexcept;
	size_type length() const noexcept;
	size_type max_size() const noexcept;
	size_type capacity() const noexcept;
	void resize(size_type count, CharT ch);
	void resize(size_type n);
	void reserve(size_type new_cap = 0);
	void shrink_to_fit();
	void clear();

	/* Modifiers */
	basic_string &erase(size_type index = 0, size_type count = npos);
	iterator erase(const_iterator pos);
	iterator erase(const_iterator first, const_iterator last);

	basic_string &append(size_type count, CharT ch);
	basic_string &append(const basic_string &str);
	basic_string &append(const basic_string &str, size_type pos,
			     size_type count);
	basic_string &append(const CharT *s, size_type count);
	basic_string &append(const CharT *s);
	template <typename InputIt,
		  typename Enable = typename pmem::detail::is_input_iterator<
			  InputIt>::type>
	basic_string &append(InputIt first, InputIt last);
	basic_string &append(std::initializer_list<CharT> ilist);

	int compare(const basic_string &other) const;
	int compare(const std::basic_string<CharT> &other) const;
	int compare(size_type pos, size_type count,
		    const basic_string &other) const;
	int compare(size_type pos, size_type count,
		    const std::basic_string<CharT> &other) const;
	int compare(size_type pos1, size_type count1, const basic_string &other,
		    size_type pos2, size_type count2 = npos) const;
	int compare(size_type pos1, size_type count1,
		    const std::basic_string<CharT> &other, size_type pos2,
		    size_type count2 = npos) const;
	int compare(const CharT *s) const;
	int compare(size_type pos, size_type count, const CharT *s) const;
	int compare(size_type pos, size_type count1, const CharT *s,
		    size_type count2) const;

	/* Special value. The exact meaning depends on the context. */
	static const size_type npos = static_cast<size_type>(-1);

private:
	using sso_type = array<value_type, sso_capacity + sizeof('\0')>;
	using non_sso_type = vector<value_type>;

	/**
	 * This union holds sso data inside of an array and non sso data inside
	 * a vector. If vector is used, it must be manually created and
	 * destroyed.
	 */
	union {
		sso_type data_sso;

		non_sso_type data_large;
	};

	/* Holds size for sso string, LSB indicates if sso is used */
	p<unsigned char> _size;

	static constexpr unsigned char _sso_mask = 0x01;

	/* helper functions */
	bool is_sso_used() const;
	void destroy_data();
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	size_type get_size(InputIt first, InputIt last) const;
	size_type get_size(size_type count, value_type ch) const;
	size_type get_size(const basic_string &other) const;
	template <typename... Args>
	pointer replace(Args &&... args);
	template <typename... Args>
	pointer initialize(Args &&... args);
	void allocate(size_type capacity);
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	pointer assign_sso_data(InputIt first, InputIt last);
	pointer assign_sso_data(size_type count, value_type ch);
	pointer assign_sso_data(basic_string &&other);
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	pointer assign_large_data(InputIt first, InputIt last);
	pointer assign_large_data(size_type count, value_type ch);
	pointer assign_large_data(basic_string &&other);
	pool_base get_pool() const;
	void check_pmem() const;
	void check_tx_stage_work() const;
	void check_pmem_tx() const;
	void snapshot_sso() const;
	size_type get_sso_size() const;
	void enable_sso();
	void disable_sso();
	void set_size(size_type new_size);
	void sso_to_large(size_t new_capacity);
	void large_to_sso();
};

/**
 * Default constructor. Construct an empty container.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string()
{
	check_pmem_tx();

	allocate(0);
	initialize(0U, value_type('\0'));
}

/**
 * Construct the container with count copies of elements with value ch.
 *
 * @param[in] count number of elements to construct.
 * @param[in] ch value of all constructed elements.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(size_type count, CharT ch)
{
	check_pmem_tx();

	allocate(count);
	initialize(count, ch);
}

/**
 * Construct the string with a substring
 * [pos, min(pos+count, other.size()) of other.
 *
 * @param[in] other string from which substring will be copied.
 * @param[in] pos start position of substring in other.
 * @param[in] count length of substring.
 *
 * @pre must be called in transaction scope.
 *
 * @throw std::out_of_range is pos > other.size()
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const basic_string &other,
					  size_type pos, size_type count)
{
	check_pmem_tx();

	if (pos > other.size())
		throw std::out_of_range("Index out of range.");

	if (count == npos || pos + count > other.size())
		count = other.size() - pos;

	auto first = static_cast<difference_type>(pos);
	auto last = first + static_cast<difference_type>(count);

	allocate(count);
	initialize(other.cbegin() + first, other.cbegin() + last);
}

/**
 * Construct the string with a substring
 * [pos, min(pos+count, other.size()) of std::basic_string<CharT> other.
 *
 * @param[in] other std::basic_string<CharT> from which substring will
 * be copied.
 * @param[in] pos start position of substring in other.
 * @param[in] count length of substring.
 *
 * @pre must be called in transaction scope.
 *
 * @throw std::out_of_range is pos > other.size()
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const std::basic_string<CharT> &other,
					  size_type pos, size_type count)
{
	check_pmem_tx();

	if (pos > other.size())
		throw std::out_of_range("Index out of range.");

	if (count == npos || pos + count > other.size())
		count = other.size() - pos;

	auto first = static_cast<difference_type>(pos);
	auto last = first + static_cast<difference_type>(count);

	allocate(count);
	initialize(other.cbegin() + first, other.cbegin() + last);
}

/**
 * Construct the string with the first count elements of C-style
 * string s.
 *
 * @param[in] s pointer to source string.
 * @param[in] count length of the resulting string.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const CharT *s, size_type count)
{
	check_pmem_tx();

	allocate(count);
	initialize(s, s + count);
}

/**
 * Construct the string with the contents of s.
 *
 * @param[in] s pointer to source string.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const CharT *s)
{
	check_pmem_tx();

	auto length = traits_type::length(s);

	allocate(length);
	initialize(s, s + length);
}

/**
 * Construct the string with the contents of the range [first, last).
 * This constructor only participates in overload resolution if InputIt
 * satisfies InputIterator.
 *
 * @param[in] first iterator to beginning of the range.
 * @param[in] last iterator to end of the range.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
basic_string<CharT, Traits>::basic_string(InputIt first, InputIt last)
{
	auto len = std::distance(first, last);
	assert(len >= 0);

	check_pmem_tx();

	allocate(static_cast<size_type>(len));
	initialize(first, last);
}

/**
 * Copy constructor. Construct the string with the copy of the contents
 * of other.
 *
 * @param[in] other reference to the string to be copied.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const basic_string &other)
{
	check_pmem_tx();

	allocate(other.size());
	initialize(other.cbegin(), other.cend());
}

/**
 * Copy constructor. Construct the string with the copy of the contents
 * of std::basic_string<CharT> other.
 *
 * @param[in] other reference to the std::basic_string<CharT> to be
 * copied.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const std::basic_string<CharT> &other)
    : basic_string(other.cbegin(), other.cend())
{
}

/**
 * Move constructor. Construct the string with the contents of other
 * using move semantics.
 *
 * @param[in] other rvalue reference to the string to be moved from.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(basic_string &&other)
{
	check_pmem_tx();

	allocate(other.size());
	initialize(std::move(other));

	if (other.is_sso_used()) {
		other.set_size(0);
		other.initialize(0U, value_type('\0'));
	}
}

/**
 * Construct the container with the contents of the initializer list
 * init.
 *
 * @param[in] ilist initializer list with content to be constructed.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 * @throw pmem::transaction_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(std::initializer_list<CharT> ilist)
{
	check_pmem_tx();

	allocate(ilist.size());
	initialize(ilist.begin(), ilist.end());
}

/**
 * Destructor.
 *
 * XXX: implement free_data()
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::~basic_string()
{
	if (!is_sso_used())
		detail::destroy<non_sso_type>(data_large);
}

/**
 * Copy assignment operator. Replace the string with contents of other
 * transactionally.
 *
 * @param[in] other reference to the string to be copied.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator=(const basic_string &other)
{
	return assign(other);
}

/**
 * Copy assignment operator. Replace the string with contents of
 * std::basic_string<CharT> other.
 *
 * @param[in] other reference to the std::basic_string<CharT> to be
 * copied.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator=(const std::basic_string<CharT> &other)
{
	return assign(other);
}

/**
 * Move assignment operator. Replace the string with the contents of
 * other using move semantics transactionally.
 *
 * @param[in] other rvalue reference to the string to be moved from.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator=(basic_string &&other)
{
	return assign(std::move(other));
}

/**
 * Replace the contents with copy of C-style string s transactionally.
 *
 * @param[in] s pointer to source string.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator=(const CharT *s)
{
	return assign(s);
}

/**
 * Replace the contents with character ch transactionally.
 *
 * @param[in] ch character.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator=(CharT ch)
{
	return assign(1, ch);
}

/**
 * Replace the contents with those of the initializer list ilist
 * transactionally.
 *
 * @param[in] ilist initializer_list of characters.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator=(std::initializer_list<CharT> ilist)
{
	return assign(ilist);
}

/**
 * Replace the contents with count copies of character ch
 * transactionally.
 *
 * @param[in] count number of characters.
 * @param[in] ch character.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(size_type count, CharT ch)
{
	auto pop = get_pool();

	transaction::run(pop, [&] { replace(count, ch); });

	return *this;
}

/**
 * Replace the string with the copy of the contents of other
 * transactionally.
 *
 * @param[in] other reference to the string to be copied.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(const basic_string &other)
{
	if (&other == this)
		return *this;

	auto pop = get_pool();

	transaction::run(pop, [&] { replace(other.cbegin(), other.cend()); });

	return *this;
}

/**
 * Replace the string with the copy of the contents of
 * std::basic_string<CharT> other.
 *
 * @param[in] other reference to the std::basic_string<CharT> to be
 * copied.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(const std::basic_string<CharT> &other)
{
	return assign(other.cbegin(), other.cend());
}

/**
 * Replace the contents with a substring
 * [pos, std::min(pos+count, other.size()) of other transactionally.
 *
 * @param[in] other string from which substring will be copied.
 * @param[in] pos start position of substring in other.
 * @param[in] count length of substring.
 *
 * @throw std::out_of_range is pos > other.size()
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(const basic_string &other, size_type pos,
				    size_type count)
{
	if (pos > other.size())
		throw std::out_of_range("Index out of range.");

	if (count == npos || pos + count > other.size())
		count = other.size() - pos;

	auto pop = get_pool();
	auto first = static_cast<difference_type>(pos);
	auto last = first + static_cast<difference_type>(count);

	transaction::run(pop, [&] {
		replace(other.cbegin() + first, other.cbegin() + last);
	});

	return *this;
}

/**
 * Replace the contents with a substring
 * [pos, std::min(pos+count, other.size()) of std::basic_string<CharT>
 * other transactionally.
 *
 * @param[in] other std::basic_string<CharT> from which substring will
 * be copied.
 * @param[in] pos start position of substring in other.
 * @param[in] count length of substring.
 *
 * @throw std::out_of_range is pos > other.size()
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(const std::basic_string<CharT> &other,
				    size_type pos, size_type count)
{
	if (pos > other.size())
		throw std::out_of_range("Index out of range.");

	if (count == npos || pos + count > other.size())
		count = other.size() - pos;

	return assign(other.c_str() + pos, count);
}

/**
 * Replace the contents with the first count elements of C-style string
 * s transactionally.
 *
 * @param[in] s pointer to source string.
 * @param[in] count length of the string.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(const CharT *s, size_type count)
{
	auto pop = get_pool();

	transaction::run(pop, [&] { replace(s, s + count); });

	return *this;
}

/**
 * Replace the contents with copy of C-style string s transactionally.
 *
 * @param[in] s pointer to source string.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(const CharT *s)
{
	auto pop = get_pool();

	auto length = traits_type::length(s);

	transaction::run(pop, [&] { replace(s, s + length); });

	return *this;
}

/**
 * Replace the contents with copies of elements in the range [first,
 * last) transactionally. This function participates in overload
 * resolution only if InputIt satisfies InputIterator.
 *
 * @param[in] first iterator to beginning of the range.
 * @param[in] last iterator to end of the range.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(InputIt first, InputIt last)
{
	auto pop = get_pool();

	transaction::run(pop, [&] { replace(first, last); });

	return *this;
}

/**
 * Replace the string with the contents of other using move semantics
 * transactionally. Other is left in valid state with size equal to 0.
 *
 * @param[in] other rvalue reference to the string to be moved from.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(basic_string &&other)
{
	if (&other == this)
		return *this;

	auto pop = get_pool();

	transaction::run(pop, [&] {
		replace(std::move(other));

		if (other.is_sso_used()) {
			other.set_size(0);
			other.initialize(0U, value_type('\0'));
		}
	});

	return *this;
}

/**
 * Replaces the contents with those of the initializer list ilist
 * transactionally.
 *
 * @param[in] ilist initializer_list of characters.
 *
 * @throw pmem::transaction_alloc_error when allocating memory for
 * underlying storage in transaction failed.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::assign(std::initializer_list<CharT> ilist)
{
	return assign(ilist.begin(), ilist.end());
}

/**
 * Return an iterator to the beginning.
 *
 * @return an iterator pointing to the first element in the string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::begin()
{
	return is_sso_used() ? iterator(&*data_sso.begin())
			     : iterator(&*data_large.begin());
}

/**
 * Return const iterator to the beginning.
 *
 * @return const iterator pointing to the first element in the string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_iterator
basic_string<CharT, Traits>::begin() const noexcept
{
	return cbegin();
}

/**
 * Return const iterator to the beginning.
 *
 * @return const iterator pointing to the first element in the string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_iterator
basic_string<CharT, Traits>::cbegin() const noexcept
{
	return is_sso_used() ? const_iterator(&*data_sso.cbegin())
			     : const_iterator(&*data_large.cbegin());
}

/**
 * Return an iterator to past the end.
 *
 * @return iterator referring to the past-the-end element in the string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::end()
{
	return begin() + static_cast<difference_type>(size());
}

/**
 * Return const iterator to past the end.
 *
 * @return const_iterator referring to the past-the-end element in the
 * string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_iterator
basic_string<CharT, Traits>::end() const noexcept
{
	return cbegin() + static_cast<difference_type>(size());
}

/**
 * Return const iterator to past the end.
 *
 * @return const_iterator referring to the past-the-end element in the
 * string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_iterator
basic_string<CharT, Traits>::cend() const noexcept
{
	return cbegin() + static_cast<difference_type>(size());
}

/**
 * Return a reverse iterator to the beginning.
 *
 * @return a reverse iterator pointing to the last element in
 * non-reversed string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::reverse_iterator
basic_string<CharT, Traits>::rbegin()
{
	return reverse_iterator(end());
}

/**
 * Return a const reverse iterator to the beginning.
 *
 * @return a const reverse iterator pointing to the last element in
 * non-reversed string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_reverse_iterator
basic_string<CharT, Traits>::rbegin() const noexcept
{
	return crbegin();
}

/**
 * Return a const reverse iterator to the beginning.
 *
 * @return a const reverse iterator pointing to the last element in
 * non-reversed string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_reverse_iterator
basic_string<CharT, Traits>::crbegin() const noexcept
{
	return const_reverse_iterator(cend());
}

/**
 * Return a reverse iterator to the end.
 *
 * @return reverse iterator referring to character preceding first
 * character in the non-reversed string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::reverse_iterator
basic_string<CharT, Traits>::rend()
{
	return reverse_iterator(begin());
}

/**
 * Return a const reverse iterator to the end.
 *
 * @return const reverse iterator referring to character preceding
 * first character in the non-reversed string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_reverse_iterator
basic_string<CharT, Traits>::rend() const noexcept
{
	return crend();
}

/**
 * Return a const reverse iterator to the end.
 *
 * @return const reverse iterator referring to character preceding
 * first character in the non-reversed string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_reverse_iterator
basic_string<CharT, Traits>::crend() const noexcept
{
	return const_reverse_iterator(cbegin());
}

/**
 * Access element at specific index with bounds checking and snapshot it
 * if there is an active transaction.
 *
 * @param[in] n index number.
 *
 * @return reference to element number n in underlying array.
 *
 * @throw std::out_of_range if n is not within the range of the
 * container.
 * @throw pmem::transaction_error when adding the object to the
 * transaction failed.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::reference
basic_string<CharT, Traits>::at(size_type n)
{
	if (n >= size())
		throw std::out_of_range("string::at");

	return is_sso_used() ? data_sso[n] : data_large[n];
}

/**
 * Access element at specific index with bounds checking.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying array.
 *
 * @throw std::out_of_range if n is not within the range of the
 * container.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_reference
basic_string<CharT, Traits>::at(size_type n) const
{
	return const_at(n);
}

/**
 * Access element at specific index with bounds checking. In
 * contradiction to at(), const_at() will return const_reference not
 * depending on the const-qualification of the object it is called on.
 * std::basic_string doesn't provide const_at() method.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying array.
 *
 * @throw std::out_of_range if n is not within the range of the
 * container.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_reference
basic_string<CharT, Traits>::const_at(size_type n) const
{
	if (n >= size())
		throw std::out_of_range("string::const_at");

	return is_sso_used() ? static_cast<const sso_type &>(data_sso)[n]
			     : static_cast<const non_sso_type &>(data_large)[n];
}

/**
 * Access element at specific index and snapshot it if there is an
 * active transaction. No bounds checking is performed.
 *
 * @param[in] n index number.
 *
 * @return reference to element number n in underlying array.
 *
 * @throw pmem::transaction_error when adding the object to the
 * transaction failed.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::reference basic_string<CharT, Traits>::
operator[](size_type n)
{
	return is_sso_used() ? data_sso[n] : data_large[n];
}

/**
 * Access element at specific index. No bounds checking is performed.
 *
 * @param[in] n index number.
 *
 * @return const_reference to element number n in underlying array.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::const_reference
	basic_string<CharT, Traits>::operator[](size_type n) const
{
	return is_sso_used() ? data_sso[n] : data_large[n];
}

/**
 * Access first element and snapshot it if there is an
 * active transaction.
 *
 * @return reference to first element in string.
 *
 * @throw pmem::transaction_error when adding the object to the
 * transaction failed.
 */
template <typename CharT, typename Traits>
CharT &
basic_string<CharT, Traits>::front()
{
	return (*this)[0];
}

/**
 * Access first element.
 *
 * @return const reference to first element in string.
 */
template <typename CharT, typename Traits>
const CharT &
basic_string<CharT, Traits>::front() const
{
	return cfront();
}

/**
 * Access first element. In contradiction to front(), cfront() will
 * return const_reference not depending on the const-qualification of
 * the object it is called on. std::basic_string doesn't provide
 * cfront() method.
 *
 * @return const reference to first element in string.
 */
template <typename CharT, typename Traits>
const CharT &
basic_string<CharT, Traits>::cfront() const
{
	return static_cast<const basic_string &>(*this)[0];
}

/**
 * Access last element and snapshot it if there is an
 * active transaction.
 *
 * @return reference to last element in string.
 *
 * @throw pmem::transaction_error when adding the object to the
 * transaction failed.
 */
template <typename CharT, typename Traits>
CharT &
basic_string<CharT, Traits>::back()
{
	return (*this)[size() - 1];
}

/**
 * Access last element.
 *
 * @return const reference to last element in string.
 */
template <typename CharT, typename Traits>
const CharT &
basic_string<CharT, Traits>::back() const
{
	return cback();
}

/**
 * Access last element. In contradiction to back(), cback() will return
 * const_reference not depending on the const-qualification of the
 * object it is called on. std::basic_string doesn't provide
 * cback() method.
 *
 * @return const reference to last element in string.
 */
template <typename CharT, typename Traits>
const CharT &
basic_string<CharT, Traits>::cback() const
{
	return static_cast<const basic_string &>(*this)[size() - 1];
}

/**
 * @return number of CharT elements in the string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::size() const noexcept
{
	if (is_sso_used())
		return get_sso_size();
	else if (data_large.size() == 0)
		return 0;
	else
		return data_large.size() - sizeof('\0');
}

/**
 * @return pointer to underlying data.
 *
 * @throw transaction_error when adding data to the
 * transaction failed.
 */
template <typename CharT, typename Traits>
CharT *
basic_string<CharT, Traits>::data()
{
	return is_sso_used() ? data_sso.range(0, size() + sizeof('\0')).begin()
			     : data_large.data();
}

/**
 * Remove characters from string starting at index transactionally.
 * Length of the string to erase is determined as the smaller of count and
 * size() - index.
 *
 * @param[in] index first character to remove.
 * @param[in] count number of characters to remove.
 *
 * @return *this
 *
 * @pre index <= size()
 *
 * @post size() = size() - std::min(count, size() - index)
 *
 * @throw std::out_of_range if index > size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::erase(size_type index, size_type count)
{
	auto sz = size();

	assert(index <= sz);

	if (index > sz)
		throw std::out_of_range("Index exceeds size");

	auto len = std::min(count, sz - index);

	auto pop = get_pool();

	auto first = begin() + static_cast<difference_type>(index);
	auto last = first + static_cast<difference_type>(len);

	transaction::run(pop, [&] {
		if (is_sso_used()) {
			auto dest = data_sso.range(index, len + sizeof('\0'))
					    .begin();

			traits_type::move(dest, &*last, len);

			auto new_size = sz - len;
			set_size(new_size);
			data_sso[new_size] = value_type('\0');
		} else {
			data_large.erase(first, last);
		}
	});

	return *this;
};

/**
 * Remove character from string at pos position transactionally.
 *
 * @param[in] pos position of character to be removed.
 *
 * @return Iterator following the removed element. If the iterator pos
 * refers to the last element, the end() iterator is returned.
 *
 * @pre pos <= size()
 *
 * @post size() = size() - 1
 *
 * @throw std::out_of_range if pos > size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::erase(const_iterator pos)
{
	auto begin = cbegin();
	size_type index = static_cast<size_type>(std::distance(begin, pos));

	erase(index, 1);

	return begin + index;
};

/**
 * Remove characters from string at [first, last) range transactionally.
 *
 * @param[in] first begin of the range of characters to be removed.
 * @param[in] last end of the range of characters to be removed.
 *
 * @return Iterator which points to the element pointed by the last iterator
 * before the erase operation. If no such element exists then end() iterator is
 * returned.
 *
 * @pre first and last are valid iterators on *this.
 *
 * @post size() = size() - std::distance(first, last)
 *
 * @throw std::out_of_range if [first, last) is not a valid range of *this.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::erase(const_iterator first, const_iterator last)
{
	auto beg = begin();

	size_type index =
		static_cast<size_type>(std::distance(cbegin(), first));
	size_type len = static_cast<size_type>(std::distance(first, last));

	erase(index, len);

	return beg + static_cast<difference_type>(index);
};

/**
 * Append count copies of character ch to the string transactionally.
 *
 * @param[in] count number of characters to append.
 * @param[in] ch character value to append.
 *
 * @return *this
 *
 * @post size() == size() + count
 * @post capacity() == std::max(size() + count, capacity())
 *
 * @throw std::length_error if size() + count > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(size_type count, CharT ch)
{
	auto sz = size();
	auto new_size = sz + count;

	if (new_size > max_size())
		throw std::length_error("Size exceeds max size");

	auto pop = get_pool();

	transaction::run(pop, [&] {
		if (is_sso_used()) {
			if (new_size > sso_capacity) {
				sso_to_large(new_size);
				data_large.resize(new_size, ch);
				data_large.push_back(value_type('\0'));
			} else {
				snapshot_sso();
				auto dest =
					data_sso.range(sz, count + sizeof('\0'))
						.begin();
				traits_type::assign(dest, count, ch);

				set_size(new_size);
				data_sso[new_size] = '\0';
			}
		} else {
			data_large.resize(new_size, ch);
			data_large.push_back(value_type('\0'));
		}
	});

	return *this;
}

/**
 * Append string str transactionally.
 *
 * @param[in] str string to append.
 *
 * @return *this
 *
 * @post size() == size() + str.size()
 * @post capacity() == std::max(size() + str.size(), capacity())
 *
 * @throw std::length_error if size() + str.size() > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(const basic_string &str)
{
	append(str.data(), str.size());

	return *this;
}

/**
 * Append substring [pos, pos + count) of str string transactionally.
 * Length of the string to append is determined as the smaller of count and
 * str.size() - pos.
 *
 * @param[in] str string to append.
 * @param[in] pos index of the first character to append.
 * @param[in] count characters to append.
 *
 * @return *this
 *
 * @pre pos <= str.size()
 *
 * @post size() == size() + std::min(count, str.size() - pos)
 * @post capacity() == std::max(size() + std::min(count, str.size() - pos),
 * capacity())
 *
 * @throw std::out_of_range if pos > str.size()
 * @throw std::length_error if size() + count > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(const basic_string &str, size_type pos,
				    size_type count)
{
	auto sz = str.size();
	assert(pos <= sz);

	if (pos > sz)
		throw std::out_of_range("Index out of range.");

	auto len = std::min(count, sz - pos);

	append(str.data() + pos, len);

	return *this;
}

/**
 * Append characters in the range [s, s + count) transactionally.
 *
 * @param[in] s pointer to C-style string to append.
 * @param[in] count characters to append.
 *
 * @return *this
 *
 * @post size() == size() + count
 * @post capacity() == std::max(size() + count, capacity())
 *
 * @throw std::length_error if size() + count > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(const CharT *s, size_type count)
{
	auto sz = size();
	auto new_size = sz + count;

	if (new_size > max_size())
		throw std::length_error("Size exceeds max size");

	auto pop = get_pool();

	transaction::run(pop, [&] {
		if (is_sso_used()) {
			if (new_size > sso_capacity) {
				sso_to_large(new_size);
				data_large.insert(
					data_large.begin() +
						static_cast<ptrdiff_t>(sz),
					s, s + count);
				data_large.push_back(value_type('\0'));
			} else {
				snapshot_sso();
				auto dest =
					data_sso.range(sz,
						       new_size + sizeof('\0'))
						.begin();
				traits_type::copy(dest, s, count);

				set_size(new_size);
				data_sso[new_size] = '\0';
			}
		} else {
			data_large.insert(data_large.end(), s, s + count);
			data_large.push_back(value_type('\0'));
		}
	});

	return *this;
}

/**
 * Append C-style string transactionally.
 * Length of the string is determined by the first null character.
 *
 * @param[in] s pointer to C-style string to append.
 *
 * @return *this
 *
 * @post size() == size() + traits::length(s)
 * @post capacity() == std::max(size() + traits::length(s), capacity())
 *
 * @throw std::length_error if size() + traits::length(s) > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(const CharT *s)
{
	append(s, traits_type::length(s));

	return *this;
}

/**
 * Append characters in the range [first, last) transactionally.
 * This overload participates in overload resolution only if
 * InputIt qualifies as InputIterator.
 *
 * @param[in] first begin of the range of characters to append.
 * @param[in] last end of the range of characters to append.
 *
 * @return *this
 *
 * @post size() == size() + std::distance(first, last)
 * @post capacity() == std::max(size() + std::distance(first, last), capacity())
 *
 * @throw std::length_error if size() + std::distance(first, last) > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(InputIt first, InputIt last)
{
	const std::basic_string<CharT, Traits> tmp(first, last);

	append(tmp.data(), tmp.size());

	return *this;
}

/**
 * Append characters from the ilist initializer list.
 *
 * @param[in] ilist initializer list with characters to append from
 *
 * @return *this
 *
 * @post size() == size() + std::distance(ilist.begin(), ilist.end())
 * @post capacity() == std::max(size() + std::distance(ilist.begin(),
 * ilist.end()), capacity())
 *
 * @throw std::length_error if size() + std::distance(ilist.begin(),
 * ilist.end()) > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(std::initializer_list<CharT> ilist)
{
	append(ilist.begin(), ilist.end());

	return *this;
};

/**
 * Compares [pos, pos + count1) substring of this to
 * [s, s + count2) substring of s.
 *
 * If count > size() - pos, substring is equal to [pos, size()).
 *
 * @param[in] pos beginning of substring of this.
 * @param[in] count1 length of substring of this.
 * @param[in] s C-style string to compare to.
 * @param[in] count2 length of substring of s.
 *
 * @return negative value if substring of *this < substring of s in
 * lexicographical order, zero if substring of *this == substring of
 * s and positive value if substring of *this > substring of s.
 *
 * @throw std::out_of_range is pos > size()
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(size_type pos, size_type count1,
				     const CharT *s, size_type count2) const
{
	if (pos > size())
		throw std::out_of_range("Index out of range.");

	if (count1 > size() - pos)
		count1 = size() - pos;

	auto ret = traits_type::compare(cdata() + pos, s,
					std::min<size_type>(count1, count2));

	if (ret != 0)
		return ret;

	if (count1 < count2)
		return -1;
	else if (count1 == count2)
		return 0;
	else
		return 1;
}

/**
 * Compares this string to other.
 *
 * @param[in] other string to compare to.
 *
 * @return negative value if *this < other in lexicographical order,
 * zero if *this == other and positive value if *this > other.
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(const basic_string &other) const
{
	return compare(0, size(), other.cdata(), other.size());
}

/**
 * Compares this string to std::basic_string<CharT> other.
 *
 * @param[in] other std::basic_string<CharT> to compare to.
 *
 * @return negative value if *this < other in lexicographical order,
 * zero if *this == other and positive value if *this > other.
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(
	const std::basic_string<CharT> &other) const
{
	return compare(0, size(), other.data(), other.size());
}

/**
 * Compares [pos, pos + count) substring of this to other.
 * If count > size() - pos, substring is equal to [pos, size()).
 *
 * @param[in] pos beginning of the substring.
 * @param[in] count length of the substring.
 * @param[in] other string to compare to.
 *
 * @return negative value if substring < other in lexicographical order,
 * zero if substring == other and positive value if substring > other.
 *
 * @throw std::out_of_range is pos > size()
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(size_type pos, size_type count,
				     const basic_string &other) const
{
	return compare(pos, count, other.cdata(), other.size());
}

/**
 * Compares [pos, pos + count) substring of this to
 * std::basic_string<CharT> other. If count > size() - pos, substring is
 * equal to [pos, size()).
 *
 * @param[in] pos beginning of the substring.
 * @param[in] count length of the substring.
 * @param[in] other std::basic_string<CharT> to compare to.
 *
 * @return negative value if substring < other in lexicographical order,
 * zero if substring == other and positive value if substring > other.
 *
 * @throw std::out_of_range is pos > size()
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(
	size_type pos, size_type count,
	const std::basic_string<CharT> &other) const
{
	return compare(pos, count, other.data(), other.size());
}

/**
 * Compares [pos1, pos1 + count1) substring of this to
 * [pos2, pos2 + count2) substring of other.
 *
 * If count1 > size() - pos, substring is equal to [pos1, size()).
 *
 * @param[in] pos1 beginning of substring of this.
 * @param[in] count1 length of substring of this.
 * @param[in] other string to compare to.
 * @param[in] pos2 beginning of substring of other.
 * @param[in] count2 length of substring of other.
 *
 * @return negative value if substring of *this < substring of other in
 * lexicographical order, zero if substring of *this == substring of
 * other and positive value if substring of *this > substring of other.
 *
 * @throw std::out_of_range is pos1 > size() or pos2 > other.size()
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(size_type pos1, size_type count1,
				     const basic_string &other, size_type pos2,
				     size_type count2) const
{
	if (pos2 > other.size())
		throw std::out_of_range("Index out of range.");

	if (count2 > other.size() - pos2)
		count2 = other.size() - pos2;

	return compare(pos1, count1, other.cdata() + pos2, count2);
}

/**
 * Compares [pos1, pos1 + count1) substring of this to
 * [pos2, pos2 + count2) substring of std::basic_string<CharT> other.
 *
 * If count1 > size() - pos, substring is equal to [pos1, size()).
 *
 * @param[in] pos1 beginning of substring of this.
 * @param[in] count1 length of substring of this.
 * @param[in] other std::basic_string<CharT> to compare to.
 * @param[in] pos2 beginning of substring of other.
 * @param[in] count2 length of substring of other.
 *
 * @return negative value if substring of *this < substring of other in
 * lexicographical order, zero if substring of *this == substring of
 * other and positive value if substring of *this > substring of other.
 *
 * @throw std::out_of_range is pos1 > size() or pos2 > other.size()
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(size_type pos1, size_type count1,
				     const std::basic_string<CharT> &other,
				     size_type pos2, size_type count2) const
{
	if (pos2 > other.size())
		throw std::out_of_range("Index out of range.");

	if (count2 > other.size() - pos2)
		count2 = other.size() - pos2;

	return compare(pos1, count1, other.data() + pos2, count2);
}

/**
 * Compares this string to s.
 *
 * @param[in] s C-style string to compare to.
 *
 * @return negative value if *this < s in lexicographical order,
 * zero if *this == s and positive value if *this > s.
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(const CharT *s) const
{
	return compare(0, size(), s, traits_type::length(s));
}

/**
 * Compares [pos, pos + count) substring of this to s.
 * If count > size() - pos, substring is equal to [pos, size()).
 *
 * @param[in] pos beginning of the substring.
 * @param[in] count length of the substring.
 * @param[in] s C-style string to compare to.
 *
 * @return negative value if substring < s in lexicographical order,
 * zero if substring == s and positive value if substring > s.
 *
 * @throw std::out_of_range is pos > size()
 */
template <typename CharT, typename Traits>
int
basic_string<CharT, Traits>::compare(size_type pos, size_type count,
				     const CharT *s) const
{
	return compare(pos, count, s, traits_type::length(s));
}

/**
 * @return const pointer to underlying data.
 */
template <typename CharT, typename Traits>
const CharT *
basic_string<CharT, Traits>::cdata() const noexcept
{
	return is_sso_used() ? data_sso.cdata() : data_large.cdata();
}

/**
 * @return pointer to underlying data.
 */
template <typename CharT, typename Traits>
const CharT *
basic_string<CharT, Traits>::data() const noexcept
{
	return cdata();
}

/**
 * @return pointer to underlying data.
 */
template <typename CharT, typename Traits>
const CharT *
basic_string<CharT, Traits>::c_str() const noexcept
{
	return cdata();
}

/**
 * @return number of CharT elements in the string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::length() const noexcept
{
	return size();
}

/**
 * @return maximum number of elements the string is able to hold.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::max_size() const noexcept
{
	return PMEMOBJ_MAX_ALLOC_SIZE / sizeof(CharT);
}

/**
 * @return number of characters that can be held in currently allocated
 * storage.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::capacity() const noexcept
{
	return is_sso_used() ? sso_capacity
			     : data_large.capacity() - sizeof('\0');
}

/**
 * Resize the string to count characters transactionally. If the current size
 * is greater than count, the string is reduced to its first count elements.
 * If the current size is less than count, additional characters of ch value are
 * appended.
 *
 * @param[in] count new size of the container.
 * @param[in] ch character to initialize elements.
 *
 * @post capacity() == std::max(count, capacity())
 * @post size() == count
 *
 * @throw std::length_error if count > max_size()
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::resize(size_type count, CharT ch)
{
	if (count > max_size())
		throw std::length_error("Count exceeds max size.");

	auto sz = size();

	auto pop = get_pool();

	transaction::run(pop, [&] {
		if (count > sz) {
			append(count - sz, ch);
		} else if (is_sso_used()) {
			set_size(count);
			data_sso[count] = '\0';
		} else {
			data_large.resize(count, ch);
			data_large.push_back(value_type('\0'));
		}
	});
}

/**
 * Resize the string to count characters transactionally. If the current size
 * is greater than count, the string is reduced to its first count elements.
 * If the current size is less than count, additional default-initialized
 * characters are appended.
 *
 * @param[in] count new size of the container.
 *
 * @post capacity() == std::max(count, capacity())
 * @post size() == count
 *
 * @throw std::length_error if count > max_size()
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::resize(size_type count)
{
	resize(count, CharT());
}

/**
 * Increase the capacity of the string to new_cap transactionally. If
 * new_cap is greater than the current capacity(), new storage is
 * allocated, otherwise the method does nothing. If new_cap is greater than
 * capacity(), all iterators, including the past-the-end iterator, and all
 * references to the elements are invalidated. Otherwise, no iterators or
 * references are invalidated.
 *
 * @param[in] new_cap new capacity.
 *
 * @post capacity() == max(capacity(), capacity_new)
 *
 * @throw rethrows destructor exception.
 * @throw std::length_error if new_cap > max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::reserve(size_type new_cap)
{
	if (new_cap > max_size())
		throw std::length_error("New capacity exceeds max size");

	if (new_cap < capacity())
		return;

	if (is_sso_used()) {
		sso_to_large(new_cap);
	} else {
		data_large.reserve(new_cap + sizeof('\0'));
	}
}

/**
 * Remove unused capacity transactionally. If large string is used capacity will
 * be set to current size. If sso is used nothing happens.
 *
 * @post capacity() == std::min(sso_capacity, capacity())
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when reallocating failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 * @throw rethrows destructor exception.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::shrink_to_fit()
{
	if (is_sso_used())
		return;

	if (size() <= sso_capacity) {
		large_to_sso();
	} else {
		data_large.shrink_to_fit();
	}
}

/**
 * Remove all characters from the string transactionally.
 * All pointers, references, and iterators are invalidated.
 *
 * @post size() == 0
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::clear()
{
	erase(cbegin(), cend());
}

/**
 * @return true if string is empty, false otherwise.
 */
template <typename CharT, typename Traits>
bool
basic_string<CharT, Traits>::empty() const noexcept
{
	return size() == 0;
}

template <typename CharT, typename Traits>
bool
basic_string<CharT, Traits>::is_sso_used() const
{
	return _size & _sso_mask;
}

template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::destroy_data()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (is_sso_used()) {
		snapshot_sso();
		disable_sso();
		/* data_sso destructor does not have to be called */
	} else {
		data_large.free_data();
		detail::destroy<decltype(data_large)>(data_large);
	}
}

/**
 * Overload of generic get_size method used to calculate size
 * based on provided parameters.
 *
 * Return std::distance(first, last) for pair of iterators.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::get_size(InputIt first, InputIt last) const
{
	return static_cast<size_type>(std::distance(first, last));
}

/**
 * Overload of generic get_size method used to calculate size
 * based on provided parameters.
 *
 * Return count for (count, value)
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::get_size(size_type count, value_type ch) const
{
	return count;
}

/**
 * Overload of generic get_size method used to calculate size
 * based on provided parameters.
 *
 * Return size of other basic_string
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::get_size(const basic_string &other) const
{
	return other.size();
}

/**
 * Generic function which replaces current content based on provided
 * parameters. Allowed parameters are:
 * - size_type count, CharT value
 * - InputIt first, InputIt last
 * - basic_string &&
 */
template <typename CharT, typename Traits>
template <typename... Args>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::replace(Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	auto new_size = get_size(std::forward<Args>(args)...);

	/* If data_large is used and there is enough capacity */
	if (!is_sso_used() && new_size <= capacity())
		return assign_large_data(std::forward<Args>(args)...);

	destroy_data();

	allocate(new_size);
	return initialize(std::forward<Args>(args)...);
}

/**
 * Generic function which initializes memory based on provided
 * parameters - forwards parameters to initialize function of either
 * data_large or data_sso. Allowed parameters are:
 * - size_type count, CharT value
 * - InputIt first, InputIt last
 * - basic_string &&
 *
 * @pre must be called in transaction scope.
 * @pre memory must be allocated before initialization.
 */
template <typename CharT, typename Traits>
template <typename... Args>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::initialize(Args &&... args)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (is_sso_used()) {
		return assign_sso_data(std::forward<Args>(args)...);
	} else {
		return assign_large_data(std::forward<Args>(args)...);
	}
}

/**
 * Allocate storage for container of capacity bytes.
 *
 * @pre must be called in transaction scope.
 *
 * @param[in] capacity bytes to allocate.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::allocate(size_type capacity)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (capacity <= sso_capacity) {
		enable_sso();
	}
	set_size(capacity);

	/*
	 * array is aggregate type so it's not required to call
	 * a constructor.
	 */
	if (!is_sso_used()) {
		detail::conditional_add_to_tx(&data_large);
		detail::create<decltype(data_large)>(&data_large);
		data_large.reserve(capacity + sizeof('\0'));
	}
}

/**
 * Initialize sso data. Overload for pair of iterators
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_sso_data(InputIt first, InputIt last)
{
	auto size = static_cast<size_type>(std::distance(first, last));

	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size <= sso_capacity);

	auto dest = data_sso.range(0, size + sizeof('\0')).begin();
	std::copy(first, last, dest);

	dest[size] = value_type('\0');

	return dest;
}

/**
 * Initialize sso data. Overload for (count, value).
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_sso_data(size_type count, value_type ch)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(count <= sso_capacity);

	auto dest = data_sso.range(0, count + sizeof('\0')).begin();
	traits_type::assign(dest, count, ch);

	dest[count] = value_type('\0');

	return dest;
}

/**
 * Initialize sso data. Overload for rvalue reference of basic_string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_sso_data(basic_string &&other)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	return assign_sso_data(other.cbegin(), other.cend());
}

/**
 * Initialize data_large - call constructor of data_large.
 * Overload for pair of iterators.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_large_data(InputIt first, InputIt last)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	auto size = static_cast<size_type>(std::distance(first, last));

	data_large.reserve(size + sizeof('\0'));
	data_large.assign(first, last);
	data_large.push_back(value_type('\0'));

	return data_large.data();
}

/**
 * Initialize data_large - call constructor of data_large.
 * Overload for (count, value).
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_large_data(size_type count, value_type ch)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	data_large.reserve(count + sizeof('\0'));
	data_large.assign(count, ch);
	data_large.push_back(value_type('\0'));

	return data_large.data();
}

/**
 * Initialize data_large - call constructor of data_large.
 * Overload for rvalue reference of basic_string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_large_data(basic_string &&other)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (other.is_sso_used())
		return assign_large_data(other.cbegin(), other.cend());

	data_large = std::move(other.data_large);

	return data_large.data();
}

/**
 * Return pool_base instance and assert that object is on pmem.
 */
template <typename CharT, typename Traits>
pool_base
basic_string<CharT, Traits>::get_pool() const
{
	auto pop = pmemobj_pool_by_ptr(this);
	assert(pop != nullptr);

	return pool_base(pop);
}

/**
 * @throw pmem::pool_error if an object is not in persistent memory.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::check_pmem() const
{
	if (pmemobj_pool_by_ptr(this) == nullptr)
		throw pool_error("Object is not on pmem.");
}

/**
 * @throw pmem::transaction_error if called outside of a transaction.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::check_tx_stage_work() const
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw transaction_error("Call made out of transaction scope.");
}

/**
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_error if called outside of a transaction.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::check_pmem_tx() const
{
	check_pmem();
	check_tx_stage_work();
}

/**
 * Snapshot sso data.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::snapshot_sso() const
{
/*
 * XXX: this can be optimized - only snapshot length() elements.
 */
#if LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED
	VALGRIND_MAKE_MEM_DEFINED(&data_sso, sizeof(data_sso));
#endif
	data_sso.data();
};

/**
 * Return size of sso string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::get_sso_size() const
{
	return _size >> 1;
}

/**
 * Enable sso string.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::enable_sso()
{
	/* bitwise operators take args by const&, we create temporary around
	 * static constant to avoid undefined reference linker error */
	_size |= (unsigned char)(_sso_mask);
}

/**
 * Disable sso string.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::disable_sso()
{
	/* bitwise operators take args by const&, we create temporary around
	 * static constant to avoid undefined reference linker error */
	_size &= (unsigned char)(~_sso_mask);
}

template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::set_size(size_type new_size)
{
	if (new_size <= sso_capacity)
		_size = new_size << 1 | _sso_mask;
	else
		/* LSB must be cleared */
		_size = std::numeric_limits<unsigned char>::max() - 1;
}

/**
 * Resize sso string to large string of new_capacity capacity.
 * Content of sso string is preserved and copied to the large string object.
 *
 * @pre must be called in transaction scope.
 *
 * @param[in] new_capacity capacity of constructed large string.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::sso_to_large(size_t new_capacity)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	auto sz = size();

	sso_type tmp;
	if (sz) {
		std::copy(cbegin(), cend(), &*tmp.begin());
		tmp[sz] = '\0';
	}

	destroy_data();
	allocate(new_capacity);

	if (sz) {
		initialize(tmp.cbegin(), tmp.cbegin() + sz);
	} else {
		data_large.data();
	}
};

/**
 * Resize large string to sso string of size() size.
 * Content of large string is preserved and copied to the sso string.
 *
 * @pre must be called in transaction scope.
 * @pre size() of large string must be less than or equal sso_capacity
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::large_to_sso()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(size() <= sso_capacity);

	sso_type tmp;
	std::copy(cbegin(), cbegin() + size(), &*tmp.begin());
	tmp[size()] = '\0';

	auto begin = tmp.cbegin();
	auto end = begin + size();

	destroy_data();
	initialize(begin, end);
};

/**
 * Non-member equal operator.
 */
template <class CharT, class Traits>
bool
operator==(const basic_string<CharT, Traits> &lhs,
	   const basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) == 0;
}

/**
 * Non-member not equal operator.
 */
template <class CharT, class Traits>
bool
operator!=(const basic_string<CharT, Traits> &lhs,
	   const basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) != 0;
}

/**
 * Non-member less than operator.
 */
template <class CharT, class Traits>
bool
operator<(const basic_string<CharT, Traits> &lhs,
	  const basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) < 0;
}

/**
 * Non-member less or equal operator.
 */
template <class CharT, class Traits>
bool
operator<=(const basic_string<CharT, Traits> &lhs,
	   const basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) <= 0;
}

/**
 * Non-member greater than operator.
 */
template <class CharT, class Traits>
bool
operator>(const basic_string<CharT, Traits> &lhs,
	  const basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) > 0;
}

/**
 * Non-member greater or equal operator.
 */
template <class CharT, class Traits>
bool
operator>=(const basic_string<CharT, Traits> &lhs,
	   const basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) >= 0;
}

/**
 * Non-member equal operator.
 */
template <class CharT, class Traits>
bool
operator==(const CharT *lhs, const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) == 0;
}

/**
 * Non-member not equal operator.
 */
template <class CharT, class Traits>
bool
operator!=(const CharT *lhs, const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) != 0;
}

/**
 * Non-member less than operator.
 */
template <class CharT, class Traits>
bool
operator<(const CharT *lhs, const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) > 0;
}

/**
 * Non-member less or equal operator.
 */
template <class CharT, class Traits>
bool
operator<=(const CharT *lhs, const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) >= 0;
}

/**
 * Non-member greater than operator.
 */
template <class CharT, class Traits>
bool
operator>(const CharT *lhs, const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) < 0;
}

/**
 * Non-member greater or equal operator.
 */
template <class CharT, class Traits>
bool
operator>=(const CharT *lhs, const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) <= 0;
}

/**
 * Non-member equal operator.
 */
template <class CharT, class Traits>
bool
operator==(const basic_string<CharT, Traits> &lhs, const CharT *rhs)
{
	return lhs.compare(rhs) == 0;
}

/**
 * Non-member not equal operator.
 */
template <class CharT, class Traits>
bool
operator!=(const basic_string<CharT, Traits> &lhs, const CharT *rhs)
{
	return lhs.compare(rhs) != 0;
}

/**
 * Non-member less than operator.
 */
template <class CharT, class Traits>
bool
operator<(const basic_string<CharT, Traits> &lhs, const CharT *rhs)
{
	return lhs.compare(rhs) < 0;
}

/**
 * Non-member less or equal operator.
 */
template <class CharT, class Traits>
bool
operator<=(const basic_string<CharT, Traits> &lhs, const CharT *rhs)
{
	return lhs.compare(rhs) <= 0;
}

/**
 * Non-member greater than operator.
 */
template <class CharT, class Traits>
bool
operator>(const basic_string<CharT, Traits> &lhs, const CharT *rhs)
{
	return lhs.compare(rhs) > 0;
}

/**
 * Non-member greater or equal operator.
 */
template <class CharT, class Traits>
bool
operator>=(const basic_string<CharT, Traits> &lhs, const CharT *rhs)
{
	return lhs.compare(rhs) >= 0;
}

/**
 * Non-member equal operator.
 */
template <class CharT, class Traits>
bool
operator==(const std::basic_string<CharT, Traits> &lhs,
	   const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) == 0;
}

/**
 * Non-member not equal operator.
 */
template <class CharT, class Traits>
bool
operator!=(const std::basic_string<CharT, Traits> &lhs,
	   const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) != 0;
}

/**
 * Non-member less than operator.
 */
template <class CharT, class Traits>
bool
operator<(const std::basic_string<CharT, Traits> &lhs,
	  const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) > 0;
}

/**
 * Non-member less or equal operator.
 */
template <class CharT, class Traits>
bool
operator<=(const std::basic_string<CharT, Traits> &lhs,
	   const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) >= 0;
}

/**
 * Non-member greater than operator.
 */
template <class CharT, class Traits>
bool
operator>(const std::basic_string<CharT, Traits> &lhs,
	  const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) < 0;
}

/**
 * Non-member greater or equal operator.
 */
template <class CharT, class Traits>
bool
operator>=(const std::basic_string<CharT, Traits> &lhs,
	   const basic_string<CharT, Traits> &rhs)
{
	return rhs.compare(lhs) <= 0;
}

/**
 * Non-member equal operator.
 */
template <class CharT, class Traits>
bool
operator==(const basic_string<CharT, Traits> &lhs,
	   const std::basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) == 0;
}

/**
 * Non-member not equal operator.
 */
template <class CharT, class Traits>
bool
operator!=(const basic_string<CharT, Traits> &lhs,
	   const std::basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) != 0;
}

/**
 * Non-member less than operator.
 */
template <class CharT, class Traits>
bool
operator<(const basic_string<CharT, Traits> &lhs,
	  const std::basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) < 0;
}

/**
 * Non-member less or equal operator.
 */
template <class CharT, class Traits>
bool
operator<=(const basic_string<CharT, Traits> &lhs,
	   const std::basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) <= 0;
}

/**
 * Non-member greater than operator.
 */
template <class CharT, class Traits>
bool
operator>(const basic_string<CharT, Traits> &lhs,
	  const std::basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) > 0;
}

/**
 * Non-member greater or equal operator.
 */
template <class CharT, class Traits>
bool
operator>=(const basic_string<CharT, Traits> &lhs,
	   const std::basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) >= 0;
}

} /* namespace experimental */

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_BASIC_STRING_HPP */
