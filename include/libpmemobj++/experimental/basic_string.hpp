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

	/**
	 * Default constructor. Construct an empty container.
	 *
	 * @pre must be called in transaction scope.
	 *
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_error if constructor wasn't called in
	 * transaction.
	 */
	basic_string()
	{
		check_pmem_tx();

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
	basic_string(size_type count, CharT ch)
	{
		check_pmem_tx();

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
	basic_string(const basic_string &other, size_type pos,
		     size_type count = npos)
	{
		check_pmem_tx();

		if (pos > other.size())
			throw std::out_of_range("Index out of range.");

		if (count == npos || pos + count > other.size())
			count = other.size() - pos;

		auto first = static_cast<difference_type>(pos);
		auto last = first + static_cast<difference_type>(count);

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
	basic_string(const CharT *s, size_type count)
	{
		check_pmem_tx();

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
	basic_string(const CharT *s)
	{
		check_pmem_tx();

		auto length = traits_type::length(s);

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
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	basic_string(InputIt first, InputIt last)
	{
		assert(std::distance(first, last) >= 0);

		check_pmem_tx();

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
	basic_string(const basic_string &other)
	{
		check_pmem_tx();

		initialize(other.cbegin(), other.cend());
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
	basic_string(basic_string &&other)
	{
		check_pmem_tx();

		initialize(std::move(other));

		if (other.is_sso_used())
			other.initialize(0U, value_type('\0'));
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
	basic_string(std::initializer_list<CharT> ilist)
	{
		check_pmem_tx();

		initialize(ilist.begin(), ilist.end());
	}

	/**
	 * Destructor.
	 *
	 * XXX: implement free_data()
	 */
	~basic_string()
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
	basic_string &
	operator=(const basic_string &other)
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
	basic_string &
	operator=(basic_string &&other)
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
	basic_string &
	operator=(const CharT *s)
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
	basic_string &
	operator=(CharT ch)
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
	basic_string &
	operator=(std::initializer_list<CharT> ilist)
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
	basic_string &
	assign(size_type count, CharT ch)
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
	basic_string &
	assign(const basic_string &other)
	{
		if (&other == this)
			return *this;

		auto pop = get_pool();

		transaction::run(
			pop, [&] { replace(other.cbegin(), other.cend()); });

		return *this;
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
	basic_string &
	assign(const basic_string &other, size_type pos, size_type count = npos)
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
	 * Replace the contents with the first count elements of C-style string
	 * s transactionally.
	 *
	 * @param[in] s pointer to source string.
	 * @param[in] count length of the string.
	 *
	 * @throw pmem::transaction_alloc_error when allocating memory for
	 * underlying storage in transaction failed.
	 */
	basic_string &
	assign(const CharT *s, size_type count)
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
	basic_string &
	assign(const CharT *s)
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
	template <typename InputIt,
		  typename Enable = typename pmem::detail::is_input_iterator<
			  InputIt>::type>
	basic_string &
	assign(InputIt first, InputIt last)
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
	basic_string &
	assign(basic_string &&other)
	{
		if (&other == this)
			return *this;

		auto pop = get_pool();

		transaction::run(pop, [&] {
			replace(std::move(other));

			if (other.is_sso_used())
				other.initialize(0U, value_type('\0'));
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
	basic_string &
	assign(std::initializer_list<CharT> ilist)
	{
		return assign(ilist.begin(), ilist.end());
	}

	/**
	 * Return an iterator to the beginning.
	 *
	 * @return an iterator pointing to the first element in the string.
	 */
	iterator
	begin()
	{
		return is_sso_used() ? iterator(&*data_sso.begin())
				     : iterator(&*data_large.begin());
	}

	/**
	 * Return const iterator to the beginning.
	 *
	 * @return const iterator pointing to the first element in the string.
	 */
	const_iterator
	begin() const noexcept
	{
		return cbegin();
	}

	/**
	 * Return const iterator to the beginning.
	 *
	 * @return const iterator pointing to the first element in the string.
	 */
	const_iterator
	cbegin() const noexcept
	{
		return is_sso_used() ? const_iterator(&*data_sso.cbegin())
				     : const_iterator(&*data_large.cbegin());
	}

	/**
	 * Return an iterator to past the end.
	 *
	 * @return iterator referring to the past-the-end element in the string.
	 */
	iterator
	end()
	{
		return begin() + static_cast<difference_type>(size());
	}

	/**
	 * Return const iterator to past the end.
	 *
	 * @return const_iterator referring to the past-the-end element in the
	 * string.
	 */
	const_iterator
	end() const noexcept
	{
		return cbegin() + static_cast<difference_type>(size());
	}

	/**
	 * Return const iterator to past the end.
	 *
	 * @return const_iterator referring to the past-the-end element in the
	 * string.
	 */
	const_iterator
	cend() const noexcept
	{
		return cbegin() + static_cast<difference_type>(size());
	}

	/**
	 * Return a reverse iterator to the beginning.
	 *
	 * @return a reverse iterator pointing to the last element in
	 * non-reversed string.
	 */
	reverse_iterator
	rbegin()
	{
		return reverse_iterator(end());
	}

	/**
	 * Return a const reverse iterator to the beginning.
	 *
	 * @return a const reverse iterator pointing to the last element in
	 * non-reversed string.
	 */
	const_reverse_iterator
	rbegin() const noexcept
	{
		return crbegin();
	}

	/**
	 * Return a const reverse iterator to the beginning.
	 *
	 * @return a const reverse iterator pointing to the last element in
	 * non-reversed string.
	 */
	const_reverse_iterator
	crbegin() const noexcept
	{
		return const_reverse_iterator(cend());
	}

	/**
	 * Return a reverse iterator to the end.
	 *
	 * @return reverse iterator referring to character preceding first
	 * character in the non-reversed string.
	 */
	reverse_iterator
	rend()
	{
		return reverse_iterator(begin());
	}

	/**
	 * Return a const reverse iterator to the end.
	 *
	 * @return const reverse iterator referring to character preceding
	 * first character in the non-reversed string.
	 */
	const_reverse_iterator
	rend() const noexcept
	{
		return crend();
	}

	/**
	 * Return a const reverse iterator to the end.
	 *
	 * @return const reverse iterator referring to character preceding
	 * first character in the non-reversed string.
	 */
	const_reverse_iterator
	crend() const noexcept
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
	reference
	at(size_type n)
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
	const_reference
	at(size_type n) const
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
	const_reference
	const_at(size_type n) const
	{
		if (n >= size())
			throw std::out_of_range("string::const_at");

		return is_sso_used()
			? static_cast<const sso_type &>(data_sso)[n]
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
	reference operator[](size_type n)
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
	const_reference operator[](size_type n) const
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
	CharT &
	front()
	{
		return (*this)[0];
	}

	/**
	 * Access first element.
	 *
	 * @return const reference to first element in string.
	 */
	const CharT &
	front() const
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
	const CharT &
	cfront() const
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
	CharT &
	back()
	{
		return (*this)[size() - 1];
	}

	/**
	 * Access last element.
	 *
	 * @return const reference to last element in string.
	 */
	const CharT &
	back() const
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
	const CharT &
	cback() const
	{
		return static_cast<const basic_string &>(*this)[size() - 1];
	}

	/**
	 * @return number of CharT elements in the string.
	 */
	size_type
	size() const noexcept
	{
		if (is_sso_used())
			return _size;
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
	CharT *
	data()
	{
		return is_sso_used()
			? data_sso.range(0, size() + sizeof('\0')).begin()
			: data_large.data();
	}

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
	int
	compare(size_type pos, size_type count1, const CharT *s,
		size_type count2) const
	{
		if (pos > size())
			throw std::out_of_range("Index out of range.");

		if (count1 > size() - pos)
			count1 = size() - pos;

		auto ret = traits_type::compare(
			cdata() + pos, s, std::min<size_type>(count1, count2));

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
	int
	compare(const basic_string &other) const
	{
		return compare(0, size(), other.cdata(), other.size());
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
	int
	compare(size_type pos, size_type count, const basic_string &other) const
	{
		return compare(pos, count, other.cdata(), other.size());
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
	int
	compare(size_type pos1, size_type count1, const basic_string &other,
		size_type pos2, size_type count2 = npos) const
	{
		if (pos2 > other.size())
			throw std::out_of_range("Index out of range.");

		if (count2 > other.size() - pos2)
			count2 = other.size() - pos2;

		return compare(pos1, count1, other.cdata() + pos2, count2);
	}

	/**
	 * Compares this string to s.
	 *
	 * @param[in] s C-style string to compare to.
	 *
	 * @return negative value if *this < s in lexicographical order,
	 * zero if *this == s and positive value if *this > s.
	 */
	int
	compare(const CharT *s) const
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
	int
	compare(size_type pos, size_type count, const CharT *s) const
	{
		return compare(pos, count, s, traits_type::length(s));
	}

	/**
	 * @return const pointer to underlying data.
	 */
	const CharT *
	cdata() const noexcept
	{
		return is_sso_used() ? data_sso.cdata() : data_large.cdata();
	}

	/**
	 * @return pointer to underlying data.
	 */
	const CharT *
	data() const noexcept
	{
		return cdata();
	}

	/**
	 * @return pointer to underlying data.
	 */
	const CharT *
	c_str() const noexcept
	{
		return cdata();
	}

	/**
	 * @return number of CharT elements in the string.
	 */
	size_type
	length() const noexcept
	{
		return size();
	}

	/**
	 * @return maximum number of elements the string is able to hold.
	 */
	size_type
	max_size() const noexcept
	{
		return PMEMOBJ_MAX_ALLOC_SIZE / sizeof(CharT);
	}

	/**
	 * @return number of characters that can be held in currently allocated
	 * storage.
	 */
	size_type
	capacity() const noexcept
	{
		return is_sso_used() ? sso_capacity
				     : data_large.capacity() - sizeof('\0');
	}

	/**
	 * @return true if string is empty, false otherwise.
	 */
	bool
	empty() const noexcept
	{
		return size() == 0;
	}

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

	/* Holds size if sso is used, std::numeric_limits<size_type>::max()
	 * otherwise */
	p<size_type> _size;

	bool
	is_sso_used() const
	{
		assert(_size <= sso_capacity ||
		       _size == std::numeric_limits<size_type>::max());

		return _size <= sso_capacity;
	}

	void
	destroy_data()
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		/*
		 * XXX: this can be optimized - only snapshot length() elements.
		 */
#if LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED
		VALGRIND_MAKE_MEM_DEFINED(&data_sso, sizeof(data_sso));
#endif

		if (is_sso_used()) {
			data_sso.data();
			/* data_sso constructor does not have to be called */
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
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	size_type
	get_size(InputIt first, InputIt last) const
	{
		return static_cast<size_type>(std::distance(first, last));
	}

	/**
	 * Overload of generic get_size method used to calculate size
	 * based on provided parameters.
	 *
	 * Return count for (count, value)
	 */
	size_type
	get_size(size_type count, value_type ch) const
	{
		return count;
	}

	/**
	 * Overload of generic get_size method used to calculate size
	 * based on provided parameters.
	 *
	 * Return size of other basic_string
	 */
	size_type
	get_size(const basic_string &other) const
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
	template <typename... Args>
	pointer
	replace(Args &&... args)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		auto new_size = get_size(std::forward<Args>(args)...);

		/* If data_large is used and there is enough capacity */
		if (!is_sso_used() && new_size <= capacity())
			return assign_large_data(std::forward<Args>(args)...);

		destroy_data();

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
	 */
	template <typename... Args>
	pointer
	initialize(Args &&... args)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		auto new_size = get_size(std::forward<Args>(args)...);

		if (new_size <= sso_capacity)
			_size = new_size;
		else
			_size = std::numeric_limits<size_type>::max();

		if (is_sso_used()) {
			/*
			 * array is aggregate type so it's not required to call
			 * a constructor.
			 */
			return assign_sso_data(std::forward<Args>(args)...);
		} else {
			detail::create<decltype(data_large)>(&data_large);
			return assign_large_data(std::forward<Args>(args)...);
		}
	}

	/**
	 * Initialize sso data. Overload for pair of iterators
	 */
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	pointer
	assign_sso_data(InputIt first, InputIt last)
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
	pointer
	assign_sso_data(size_type count, value_type ch)
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
	pointer
	assign_sso_data(basic_string &&other)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		return assign_sso_data(other.cbegin(), other.cend());
	}

	/**
	 * Initialize data_large - call constructor of data_large.
	 * Overload for pair of iterators.
	 */
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	pointer
	assign_large_data(InputIt first, InputIt last)
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
	pointer
	assign_large_data(size_type count, value_type ch)
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
	pointer
	assign_large_data(basic_string &&other)
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
	pool_base
	get_pool() const
	{
		auto pop = pmemobj_pool_by_ptr(this);
		assert(pop != nullptr);

		return pool_base(pop);
	}

	/**
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 */
	void
	check_pmem() const
	{
		if (pmemobj_pool_by_ptr(this) == nullptr)
			throw pool_error("Object is not on pmem.");
	}

	/**
	 * @throw pmem::transaction_error if called outside of a transaction.
	 */
	void
	check_tx_stage_work() const
	{
		if (pmemobj_tx_stage() != TX_STAGE_WORK)
			throw transaction_error(
				"Call made out of transaction scope.");
	}

	/**
	 * @throw pmem::pool_error if an object is not in persistent memory.
	 * @throw pmem::transaction_error if called outside of a transaction.
	 */
	void
	check_pmem_tx() const
	{
		check_pmem();
		check_tx_stage_work();
	}
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

} /* namespace experimental */

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_BASIC_STRING_HPP */
