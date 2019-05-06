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
	static constexpr size_type sso_capacity = (32 - 8) / sizeof(CharT) - 1;

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

	/* Modifiers */
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
	using sso_type = array<value_type, sso_capacity + 1>;
	using non_sso_type = vector<value_type>;

	/**
	 * This union holds sso data inside of an array and non sso data inside
	 * a vector. If vector is used, it must be manually created and
	 * destroyed.
	 *
	 * _size is used to store length in case when SSO is used. It is the
	 * same type as first member of data field. This means that it can be
	 * safely accessed through both sso (_size variable) and non_sso (as
	 * size in a vector) no matter which one is used.
	 *
	 * C++11 §9.2/18 says:
	 * If a standard-layout union contains two or more standard-layout
	 * structs that share a common initial sequence, and if the
	 * standard-layout union object currently contains one of these
	 * standard-layout structs, it is permitted to inspect the common
	 * initial part of any of them.
	 */
	union {
		struct {
			/*
			 * EXACTLY the same type as first member in vector
			 * Holds size for sso string, bit specified by _sso_mask
			 * indicates if sso is used.
			 */
			p<size_type> _size;

			sso_type data;
		} sso;

		struct {
			non_sso_type data;
		} non_sso;
	};

	/*
	 * MSB is used because vector is known not to use entire range of
	 * size_type.
	 */
	static constexpr size_type _sso_mask = 1ULL
		<< (std::numeric_limits<size_type>::digits - 1);

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
	void set_sso_size(size_type new_size);
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
		detail::destroy<non_sso_type>(non_sso.data);
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
	return is_sso_used() ? iterator(&*sso.data.begin())
			     : iterator(&*non_sso.data.begin());
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
	return is_sso_used() ? const_iterator(&*sso.data.cbegin())
			     : const_iterator(&*non_sso.data.cbegin());
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

	return is_sso_used() ? sso.data[n] : non_sso.data[n];
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

	return is_sso_used()
		? static_cast<const sso_type &>(sso.data)[n]
		: static_cast<const non_sso_type &>(non_sso.data)[n];
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
	return is_sso_used() ? sso.data[n] : non_sso.data[n];
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
	return is_sso_used() ? sso.data[n] : non_sso.data[n];
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
	else if (non_sso.data.size() == 0)
		return 0;
	else
		return non_sso.data.size() - 1;
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
	return is_sso_used() ? sso.data.range(0, get_sso_size() + 1).begin()
			     : non_sso.data.data();
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
	return is_sso_used() ? sso.data.cdata() : non_sso.data.cdata();
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
	return PMEMOBJ_MAX_ALLOC_SIZE / sizeof(CharT) - 1;
}

/**
 * @return number of characters that can be held in currently allocated
 * storage.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::capacity() const noexcept
{
	return is_sso_used() ? sso_capacity : non_sso.data.capacity() - 1;
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
	return sso._size & _sso_mask;
}

template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::destroy_data()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (is_sso_used()) {
		snapshot_sso();
		/* sso.data destructor does not have to be called */
	} else {
		non_sso.data.free_data();
		detail::destroy<decltype(non_sso.data)>(non_sso.data);
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

	/* If non_sso.data is used and there is enough capacity */
	if (!is_sso_used() && new_size <= capacity())
		return assign_large_data(std::forward<Args>(args)...);

	destroy_data();

	allocate(new_size);
	return initialize(std::forward<Args>(args)...);
}

/**
 * Generic function which initializes memory based on provided
 * parameters - forwards parameters to initialize function of either
 * non_sso.data or sso.data. Allowed parameters are:
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

	auto size = get_size(std::forward<Args>(args)...);

	if (is_sso_used()) {
		set_sso_size(size);
		return assign_sso_data(std::forward<Args>(args)...);
	} else {
		return assign_large_data(std::forward<Args>(args)...);
	}
}

/**
 * Allocate storage for container of capacity bytes.
 * Based on capacity determine if sso or large string is used.
 *
 * @pre data must be uninitialized.
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
	} else {
		disable_sso();
	}

	/*
	 * array is aggregate type so it's not required to call
	 * a constructor.
	 */
	if (!is_sso_used()) {
		detail::conditional_add_to_tx(&non_sso.data);
		detail::create<decltype(non_sso.data)>(&non_sso.data);
		non_sso.data.reserve(capacity + sizeof('\0'));
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

	auto dest = sso.data.range(0, size + 1).begin();
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

	auto dest = sso.data.range(0, count + 1).begin();
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
 * Initialize non_sso.data - call constructor of non_sso.data.
 * Overload for pair of iterators.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_large_data(InputIt first, InputIt last)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	auto size = static_cast<size_type>(std::distance(first, last));

	non_sso.data.reserve(size + 1);
	non_sso.data.assign(first, last);
	non_sso.data.push_back(value_type('\0'));

	return non_sso.data.data();
}

/**
 * Initialize non_sso.data - call constructor of non_sso.data.
 * Overload for (count, value).
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_large_data(size_type count, value_type ch)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	non_sso.data.reserve(count + 1);
	non_sso.data.assign(count, ch);
	non_sso.data.push_back(value_type('\0'));

	return non_sso.data.data();
}

/**
 * Initialize non_sso.data - call constructor of non_sso.data.
 * Overload for rvalue reference of basic_string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::assign_large_data(basic_string &&other)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (other.is_sso_used())
		return assign_large_data(other.cbegin(), other.cend());

	non_sso.data = std::move(other.non_sso.data);

	return non_sso.data.data();
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
	VALGRIND_MAKE_MEM_DEFINED(&sso.data, sizeof(sso.data));
#endif
	sso.data.data();
};

/**
 * Return size of sso string.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::get_sso_size() const
{
	return sso._size & ~_sso_mask;
}

/**
 * Enable sso string.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::enable_sso()
{
	/* temporary size_type must be created to avoid undefined reference
	 * linker error */
	sso._size |= (size_type)(_sso_mask);
}

/**
 * Disable sso string.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::disable_sso()
{
	sso._size &= ~_sso_mask;
}

/**
 * Set size for sso.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::set_sso_size(size_type new_size)
{
	sso._size = new_size | _sso_mask;
}

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
