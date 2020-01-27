/*
 * Copyright 2019-2020, Intel Corporation
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
#include <iterator>
#include <limits>
#include <string>

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/container/detail/contiguous_iterator.hpp>
#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/iterator_traits.hpp>
#include <libpmemobj++/detail/life.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem
{

namespace obj
{

/**
 * pmem::obj::string - persistent container with std::basic_string compatible
 * interface.
 *
 * The implementation is still missing some methods.
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
	using iterator = pmem::detail::basic_contiguous_iterator<CharT>;
	using const_iterator = const_pointer;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using for_each_ptr_function =
		std::function<void(persistent_ptr_base &)>;

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
	void for_each_ptr(for_each_ptr_function func);

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
	/* We add following overloads to resolve erase(0) ambiguity */
	template <typename T,
		  typename Enable = typename std::enable_if<
			  std::is_convertible<T, size_type>::value>::type>
	basic_string &erase(T param);
	template <typename T,
		  typename Enable = typename std::enable_if<
			  !std::is_convertible<T, size_type>::value>::type>
	iterator erase(T param);
	void pop_back();

	basic_string &append(size_type count, CharT ch);
	basic_string &append(const basic_string &str);
	basic_string &append(const basic_string &str, size_type pos,
			     size_type count = npos);
	basic_string &append(const CharT *s, size_type count);
	basic_string &append(const CharT *s);
	template <typename InputIt,
		  typename Enable = typename pmem::detail::is_input_iterator<
			  InputIt>::type>
	basic_string &append(InputIt first, InputIt last);
	basic_string &append(std::initializer_list<CharT> ilist);
	void push_back(CharT ch);
	basic_string &operator+=(const basic_string &str);
	basic_string &operator+=(const CharT *s);
	basic_string &operator+=(CharT c);
	basic_string &operator+=(std::initializer_list<CharT> ilist);

	basic_string &insert(size_type index, size_type count, CharT ch);
	basic_string &insert(size_type index, const CharT *s);
	basic_string &insert(size_type index, const CharT *s, size_type count);
	basic_string &insert(size_type index, const basic_string &str);
	basic_string &insert(size_type index1, const basic_string &str,
			     size_type index2, size_type count = npos);
	iterator insert(const_iterator pos, CharT ch);
	iterator insert(const_iterator pos, size_type count, CharT ch);
	template <typename InputIt,
		  typename Enable = typename pmem::detail::is_input_iterator<
			  InputIt>::type>
	iterator insert(const_iterator pos, InputIt first, InputIt last);
	iterator insert(const_iterator pos, std::initializer_list<CharT> ilist);
	template <typename T,
		  typename Enable = typename std::enable_if<
			  std::is_convertible<T, size_type>::value>::type>
	basic_string &insert(T param, size_type count, CharT ch);
	template <typename T,
		  typename Enable = typename std::enable_if<
			  !std::is_convertible<T, size_type>::value>::type>
	iterator insert(T param, size_type count, CharT ch);

	basic_string &replace(size_type index, size_type count,
			      const basic_string &str);
	basic_string &replace(const_iterator first, const_iterator last,
			      const basic_string &str);
	basic_string &replace(size_type index, size_type count,
			      const basic_string &str, size_type index2,
			      size_type count2 = npos);
	template <typename InputIt,
		  typename Enable = typename pmem::detail::is_input_iterator<
			  InputIt>::type>
	basic_string &replace(const_iterator first, const_iterator last,
			      InputIt first2, InputIt last2);
	basic_string &replace(const_iterator first, const_iterator last,
			      const CharT *s, size_type count2);
	basic_string &replace(const_iterator first, const_iterator last,
			      const CharT *s);
	basic_string &replace(size_type index, size_type count,
			      size_type count2, CharT ch);
	basic_string &replace(const_iterator first, const_iterator last,
			      size_type count2, CharT ch);
	basic_string &replace(size_type index, size_type count, const CharT *s,
			      size_type count2);
	basic_string &replace(size_type index, size_type count, const CharT *s);
	basic_string &replace(const_iterator first, const_iterator last,
			      std::initializer_list<CharT> ilist);

	size_type copy(CharT *s, size_type count, size_type index = 0) const;

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

	/* Search */
	size_type find(const basic_string &str, size_type pos = 0) const
		noexcept;
	size_type find(const CharT *s, size_type pos, size_type count) const;
	size_type find(const CharT *s, size_type pos = 0) const;
	size_type find(CharT ch, size_type pos = 0) const noexcept;
	size_type rfind(const basic_string &str, size_type pos = npos) const
		noexcept;
	size_type rfind(const CharT *s, size_type pos, size_type count) const;
	size_type rfind(const CharT *s, size_type pos = npos) const;
	size_type rfind(CharT ch, size_type pos = npos) const noexcept;
	size_type find_first_of(const basic_string &str,
				size_type pos = 0) const noexcept;
	size_type find_first_of(const CharT *s, size_type pos,
				size_type count) const;
	size_type find_first_of(const CharT *s, size_type pos = 0) const;
	size_type find_first_of(CharT ch, size_type pos = 0) const noexcept;
	size_type find_first_not_of(const basic_string &str,
				    size_type pos = 0) const noexcept;
	size_type find_first_not_of(const CharT *s, size_type pos,
				    size_type count) const;
	size_type find_first_not_of(const CharT *s, size_type pos = 0) const;
	size_type find_first_not_of(CharT ch, size_type pos = 0) const noexcept;
	size_type find_last_of(const basic_string &str,
			       size_type pos = npos) const noexcept;
	size_type find_last_of(const CharT *s, size_type pos,
			       size_type count) const;
	size_type find_last_of(const CharT *s, size_type pos = npos) const;
	size_type find_last_of(CharT ch, size_type pos = npos) const noexcept;
	size_type find_last_not_of(const basic_string &str,
				   size_type pos = npos) const noexcept;
	size_type find_last_not_of(const CharT *s, size_type pos,
				   size_type count) const;
	size_type find_last_not_of(const CharT *s, size_type pos = npos) const;
	size_type find_last_not_of(CharT ch, size_type pos = npos) const
		noexcept;

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

			sso_type _data;
		} sso;

		struct {
			non_sso_type _data;
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
	pointer replace_content(Args &&... args);
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
	void add_sso_to_tx(size_type first, size_type num) const;
	size_type get_sso_size() const;
	void enable_sso();
	void disable_sso();
	void set_sso_size(size_type new_size);
	void sso_to_large(size_t new_capacity);
	void large_to_sso();
	typename basic_string<CharT, Traits>::non_sso_type &non_sso_data();
	typename basic_string<CharT, Traits>::sso_type &sso_data();
	const typename basic_string<CharT, Traits>::non_sso_type &
	non_sso_data() const;
	const typename basic_string<CharT, Traits>::sso_type &sso_data() const;
};

/**
 * Default constructor. Construct an empty container.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string()
{
	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(size_type count, CharT ch)
{
	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const basic_string &other,
					  size_type pos, size_type count)
{
	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const std::basic_string<CharT> &other,
					  size_type pos, size_type count)
{
	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const CharT *s, size_type count)
{
	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const CharT *s)
{
	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
basic_string<CharT, Traits>::basic_string(InputIt first, InputIt last)
{
	auto len = std::distance(first, last);
	assert(len >= 0);

	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(const basic_string &other)
{
	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(basic_string &&other)
{
	check_pmem_tx();
	sso._size = 0;

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
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits>::basic_string(std::initializer_list<CharT> ilist)
{
	check_pmem_tx();
	sso._size = 0;

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
		detail::destroy<non_sso_type>(non_sso_data());
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

	transaction::run(pop, [&] { replace_content(count, ch); });

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

	transaction::run(
		pop, [&] { replace_content(other.cbegin(), other.cend()); });

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
		replace_content(other.cbegin() + first, other.cbegin() + last);
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

	transaction::run(pop, [&] { replace_content(s, s + count); });

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

	transaction::run(pop, [&] { replace_content(s, s + length); });

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

	transaction::run(pop, [&] { replace_content(first, last); });

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
		replace_content(std::move(other));

		if (other.is_sso_used())
			other.initialize(0U, value_type('\0'));
	});

	return *this;
}

/**
 * replace_content the contents with those of the initializer list ilist
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
 * Iterates over all internal pointers and executes a callback function
 * on each of them. In this implementation, it just calls for_each_ptr()
 * of the vector stored in SSO.
 *
 * @param func callback function to call on internal pointer.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::for_each_ptr(for_each_ptr_function func)
{
	if (!is_sso_used()) {
		non_sso._data.for_each_ptr(func);
	}
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
	return is_sso_used() ? iterator(&*sso_data().begin())
			     : iterator(&*non_sso_data().begin());
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
	return is_sso_used() ? const_iterator(&*sso_data().cbegin())
			     : const_iterator(&*non_sso_data().cbegin());
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

	return is_sso_used() ? sso_data()[n] : non_sso_data()[n];
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
		? static_cast<const sso_type &>(sso_data())[n]
		: static_cast<const non_sso_type &>(non_sso_data())[n];
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
typename basic_string<CharT, Traits>::reference
	basic_string<CharT, Traits>::operator[](size_type n)
{
	return is_sso_used() ? sso_data()[n] : non_sso_data()[n];
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
	return is_sso_used() ? sso_data()[n] : non_sso_data()[n];
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
	else if (non_sso_data().size() == 0)
		return 0;
	else
		return non_sso_data().size() - 1;
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
	return is_sso_used() ? sso_data().range(0, get_sso_size() + 1).begin()
			     : non_sso_data().data();
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

	if (index > sz)
		throw std::out_of_range("Index exceeds size.");

	count = (std::min)(count, sz - index);

	auto pop = get_pool();

	auto first = begin() + static_cast<difference_type>(index);
	auto last = first + static_cast<difference_type>(count);

	if (is_sso_used()) {
		transaction::run(pop, [&] {
			auto move_len = sz - index - count;
			auto new_size = sz - count;

			auto range = sso_data().range(index, move_len + 1);

			traits_type::move(range.begin(), &*last, move_len);

			set_sso_size(new_size);

			assert(range.end() - 1 ==
			       &sso_data()._data[index + move_len]);
			*(range.end() - 1) = value_type('\0');
		});
	} else {
		non_sso_data().erase(first, last);
	}

	return *this;
}

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
	return erase(pos, pos + 1);
}

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
	size_type index =
		static_cast<size_type>(std::distance(cbegin(), first));
	size_type len = static_cast<size_type>(std::distance(first, last));

	erase(index, len);

	return begin() + static_cast<difference_type>(index);
}

/**
 * Remove the last character from the string transactionally.
 *
 * @pre !empty()
 *
 * @post size() = size() - 1
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::pop_back()
{
	erase(size() - 1, 1);
}

/**
 * Append count copies of character ch to the string transactionally.
 *
 * @param[in] count number of characters to append.
 * @param[in] ch character value to append.
 *
 * @return *this
 *
 * @post size() == size() + count
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
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
		throw std::length_error("Size exceeds max size.");

	if (is_sso_used()) {
		auto pop = get_pool();

		transaction::run(pop, [&] {
			if (new_size > sso_capacity) {
				sso_to_large(new_size);

				non_sso_data().insert(
					non_sso_data().cbegin() +
						static_cast<difference_type>(
							sz),
					count, ch);
			} else {
				add_sso_to_tx(sz, count + 1);
				traits_type::assign(&sso_data()._data[sz],
						    count, ch);

				assert(new_size == sz + count);
				set_sso_size(new_size);
				sso_data()._data[new_size] = value_type('\0');
			}
		});
	} else {
		non_sso_data().insert(non_sso_data().cbegin() +
					      static_cast<difference_type>(sz),
				      count, ch);
	}

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
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(const basic_string &str)
{
	return append(str.data(), str.size());
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
 * @post size() == size() + std::min(count, str.size() - pos).
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if pos > str.size().
 * @throw std::length_error if new size > max_size().
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

	if (pos > sz)
		throw std::out_of_range("Index out of range.");

	count = (std::min)(count, sz - pos);

	append(str.data() + pos, count);

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
 * @post size() == size() + count.
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(const CharT *s, size_type count)
{
	return append(s, s + count);
}

/**
 * Append C-style string transactionally.
 * Length of the string is determined by the first null character.
 *
 * @param[in] s pointer to C-style string to append.
 *
 * @return *this
 *
 * @post size() == size() + traits::length(s).
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(const CharT *s)
{
	return append(s, traits_type::length(s));
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
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(InputIt first, InputIt last)
{
	auto sz = size();
	auto count = static_cast<size_type>(std::distance(first, last));
	auto new_size = sz + count;

	if (new_size > max_size())
		throw std::length_error("Size exceeds max size.");

	if (is_sso_used()) {
		auto pop = get_pool();

		transaction::run(pop, [&] {
			if (new_size > sso_capacity) {
				/* 1) Cache C-style string in case of
				 * self-append, because it will be destroyed
				 * when switching from sso to large string.
				 *
				 * 2) We cache in std::vector instead of
				 * std::string because of overload deduction
				 * ambiguity on Windows
				 */
				std::vector<value_type> str(first, last);

				sso_to_large(new_size);
				non_sso_data().insert(
					non_sso_data().cbegin() +
						static_cast<difference_type>(
							sz),
					str.begin(), str.end());
			} else {
				add_sso_to_tx(sz, count + 1);
				std::copy(first, last, &sso_data()._data[sz]);

				assert(new_size == sz + count);
				set_sso_size(new_size);
				sso_data()._data[new_size] = value_type('\0');
			}
		});
	} else {
		non_sso_data().insert(non_sso_data().cbegin() +
					      static_cast<difference_type>(sz),
				      first, last);
	}

	return *this;
}

/**
 * Append characters from the ilist initializer list transactionally.
 *
 * @param[in] ilist initializer list with characters to append from
 *
 * @return *this
 *
 * @post size() == size() + std::distance(ilist.begin(), ilist.end())
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::append(std::initializer_list<CharT> ilist)
{
	return append(ilist.begin(), ilist.end());
}

/**
 * Append character ch at the end of the string transactionally.
 *
 * @param[in] ch character to append
 *
 * @post size() == size() + 1
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::push_back(CharT ch)
{
	append(static_cast<size_type>(1), ch);
}

/**
 * Append string str transactionally.
 *
 * @param[in] str string to append.
 *
 * @return *this
 *
 * @post size() == size() + str.size()
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator+=(const basic_string &str)
{
	return append(str);
}

/**
 * Append C-style string transactionally.
 * Length of the string is determined by the first null character.
 *
 * @param[in] s pointer to C-style string to append.
 *
 * @return *this
 *
 * @post size() == size() + traits::length(s).
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator+=(const CharT *s)
{
	return append(s);
}

/**
 * Append character ch at the end of the string transactionally.
 *
 * @param[in] ch character to append
 *
 * @post size() == size() + 1
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new_size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator+=(CharT ch)
{
	push_back(ch);

	return *this;
}

/**
 * Append characters from the ilist initializer list transactionally.
 *
 * @param[in] ilist initializer list with characters to append from
 *
 * @return *this
 *
 * @post size() == size() + ilist.size()
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::operator+=(std::initializer_list<CharT> ilist)
{
	return append(ilist);
}

/**
 * Insert count copies of ch character at index transactionally.
 *
 * @param[in] index position at which the content will be inserted
 * @param[in] count number of characters to insert
 * @param[in] ch character to insert
 *
 * @return *this
 *
 * @post size() == size() + count
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::insert(size_type index, size_type count, CharT ch)
{
	if (index > size())
		throw std::out_of_range("Index out of range.");

	auto pos = cbegin() + static_cast<difference_type>(index);

	insert(pos, count, ch);

	return *this;
}

/**
 * Insert null-terminated C-style string pointed by s of the length determined
 * by the first null character at index transactionally.
 *
 * @param[in] index position at which the content will be inserted.
 * @param[in] s pointer to C-style string to insert.
 *
 * @return *this
 *
 * @post size() == size() + traits::length(s).
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::insert(size_type index, const CharT *s)
{
	return insert(index, s, traits_type::length(s));
}

/**
 * Insert characters in the range [s, s+ count) at index transactionally.
 *
 * @param[in] index position at which the content will be inserted.
 * @param[in] s pointer to C-style string to insert.
 * @param[in] count number of characters to insert.
 *
 * @return *this
 *
 * @post size() == size() + count
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::insert(size_type index, const CharT *s,
				    size_type count)
{
	if (index > size())
		throw std::out_of_range("Index out of range.");

	auto pos = cbegin() + static_cast<difference_type>(index);

	insert(pos, s, s + count);

	return *this;
}

/**
 * Insert str string at index transactionally.
 *
 * @param[in] index position at which the content will be inserted.
 * @param[in] str string to insert.
 *
 * @return *this
 *
 * @post size() == size() + str.size()
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::insert(size_type index, const basic_string &str)
{
	return insert(index, str.data(), str.size());
}

/**
 * Insert a str.substr(index2, count) string at index1 transactionally.
 *
 * @param[in] index1 position at which the content will be inserted.
 * @param[in] str string to insert.
 * @param[in] index2 position of the first character in str to insert.
 * @param[in] count number of characters to insert.
 *
 * @return *this
 *
 * @post size() == size() + std::min(count, str.size() - index2)
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index1 > size() or str.size() > index2.
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::insert(size_type index1, const basic_string &str,
				    size_type index2, size_type count)
{
	auto sz = str.size();

	if (index1 > size() || index2 > sz)
		throw std::out_of_range("Index out of range.");

	count = (std::min)(count, sz - index2);

	return insert(index1, str.data() + index2, count);
}

/**
 * Insert character ch before the character pointed by pos transactionally.
 *
 * @param[in] pos iterator before which the character will be inserted.
 * @param[in] ch character to insert.
 *
 * @return iterator to the copy of the first inserted character or pos if no
 * characters were inserted.
 *
 * @pre pos is valid iterator on *this.
 *
 * @post size() == size() + 1
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::insert(const_iterator pos, CharT ch)
{
	return insert(pos, 1, ch);
}

/**
 * Insert count copies of character ch before the character pointed by pos
 * transactionally.
 *
 * @param[in] pos iterator before which the character will be inserted.
 * @param[in] count number of characters to insert.
 * @param[in] ch character to insert.
 *
 * @return iterator to the copy of the first inserted character or pos if no
 * characters were inserted.
 *
 * @pre pos is valid iterator on *this.
 *
 * @post size() == size() + count
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::insert(const_iterator pos, size_type count,
				    CharT ch)
{
	auto sz = size();

	if (sz + count > max_size())
		throw std::length_error("Count exceeds max size.");

	auto new_size = sz + count;

	auto pop = get_pool();

	auto index = static_cast<size_type>(std::distance(cbegin(), pos));

	transaction::run(pop, [&] {
		if (is_sso_used() && new_size <= sso_capacity) {
			auto len = sz - index;

			add_sso_to_tx(index, len + count + 1);

			traits_type::move(&sso_data()._data[index + count],
					  &sso_data()._data[index], len);
			traits_type::assign(&sso_data()._data[index], count,
					    ch);

			assert(new_size == index + len + count);
			set_sso_size(new_size);
			sso_data()._data[new_size] = value_type('\0');
		} else {
			if (is_sso_used())
				sso_to_large(new_size);

			non_sso_data().insert(
				non_sso_data().begin() +
					static_cast<difference_type>(index),
				count, ch);
		}
	});

	return iterator(&data()[static_cast<difference_type>(index)]);
}

/**
 * Insert characters from [first, last) range before the character pointed by
 * pos transactionally.
 *
 * @param[in] pos iterator before which the character will be inserted.
 * @param[in] first begin of the range of characters to insert.
 * @param[in] last end of the range of characters to insert.
 *
 * @return iterator to the copy of the first inserted character or pos if no
 * characters were inserted.
 *
 * @pre pos is valid iterator on *this.
 *
 * @post size() == size() + std::distance(first, last)
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::insert(const_iterator pos, InputIt first,
				    InputIt last)
{
	auto sz = size();

	auto count = static_cast<size_type>(std::distance(first, last));

	if (sz + count > max_size())
		throw std::length_error("Count exceeds max size.");

	auto pop = get_pool();

	auto new_size = sz + count;

	auto index = static_cast<size_type>(std::distance(cbegin(), pos));

	transaction::run(pop, [&] {
		if (is_sso_used() && new_size <= sso_capacity) {
			auto len = sz - index;

			add_sso_to_tx(index, len + count + 1);

			traits_type::move(&sso_data()._data[index + count],
					  &sso_data()._data[index], len);
			std::copy(first, last, &sso_data()._data[index]);

			assert(new_size == index + len + count);
			set_sso_size(new_size);
			sso_data()._data[new_size] = value_type('\0');
		} else {
			if (is_sso_used()) {
				/* 1) Cache C-style string in case of
				 * self-insert, because it will be destroyed
				 * when switching from sso to large string.
				 *
				 * 2) We cache in std::vector instead of
				 * std::string because of overload deduction
				 * ambiguity on Windows
				 */
				std::vector<value_type> str(first, last);

				sso_to_large(new_size);
				non_sso_data().insert(
					non_sso_data().begin() +
						static_cast<difference_type>(
							index),
					str.begin(), str.end());
			} else {
				non_sso_data().insert(
					non_sso_data().begin() +
						static_cast<difference_type>(
							index),
					first, last);
			}
		}
	});

	return iterator(&data()[static_cast<difference_type>(index)]);
}

/**
 * Insert characters from initializer list ilist before the character pointed by
 * pos transactionally.
 *
 * @param[in] pos iterator before which the character will be inserted.
 * @param[in] ilist initializer list of characters to insert.
 *
 * @return iterator to the copy of the first inserted character or pos if no
 * characters were inserted.
 *
 * @pre pos is valid iterator on *this.
 *
 * @post size() == size() + ilist.size()
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::insert(const_iterator pos,
				    std::initializer_list<CharT> ilist)
{
	return insert(pos, ilist.begin(), ilist.end());
}

/**
 * Replace range [index, index + count) with the content of str string
 * transactionally.
 *
 * @param[in] index start of the substring that will be replaced.
 * @param[in] count length of the substring that will be replaced.
 * @param[in] str that is used for the replacement.
 *
 * @return *this
 *
 * @post size() == size() - (std::min)(count, size() - index) + str.size()
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(size_type index, size_type count,
				     const basic_string &str)
{
	return replace(index, count, str.data(), str.size());
}

/**
 * Replace range [first, last) with the content of str string transactionally.
 *
 * @param[in] first begin of the range of characters that will be replaced.
 * @param[in] last end of the range of characters that will be replaced.
 * @param[in] str that that will be used for replacement.
 *
 * @return *this
 *
 * @post size() == size() - std::distance(first, last) + str.size()
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(const_iterator first, const_iterator last,
				     const basic_string &str)
{
	return replace(first, last, str.data(), str.data() + str.size());
}

/**
 * Replace range [index, index + count) with the substring [index2, index2 +
 * count2) of str string transactionally. If either count2 == npos or count2
 * exceeds size of str string then substring [index2, str.size()) is used.
 *
 * @param[in] index start of the substring that will be replaced.
 * @param[in] count length of the substring that will be replaced.
 * @param[in] str that is used for the replacement.
 * @param[in] index2 start of the substring that will be used for replacement.
 * @param[in] count2 length of the substring that will be used for replacement.
 *
 * @return *this
 *
 * @post size() == size() - (std::min)(count, size() - index) +
 * (std::min)(count2, str.size() - index2)
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size() or index2 > str.size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(size_type index, size_type count,
				     const basic_string &str, size_type index2,
				     size_type count2)
{
	auto sz = str.size();

	if (index2 > sz)
		throw std::out_of_range("Index out of range.");

	count2 = (std::min)(count2, sz - index2);

	return replace(index, count, str.data() + index2, count2);
}

/**
 * Replace range [first, last) with the characters in [first2, last2) range
 * transactionally.
 *
 * @param[in] first begin of the range of characters that will be replaced.
 * @param[in] last end of the range of characters that will be replaced.
 * @param[in] first2 begin of the range of characters that will be used for
 * replacement.
 * @param[in] last2 end of the range of characters that will be used for
 * replacement.
 *
 * @return *this
 *
 * @post size() == size() - std::distance(first, last) + std::distance(first2,
 * last2).
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
template <typename InputIt, typename Enable>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(const_iterator first, const_iterator last,
				     InputIt first2, InputIt last2)
{
	auto sz = size();
	auto index = static_cast<size_type>(std::distance(cbegin(), first));
	auto count = static_cast<size_type>(std::distance(first, last));
	auto count2 = static_cast<size_type>(std::distance(first2, last2));

	count = (std::min)(count, sz - index);

	if (sz - count + count2 > max_size())
		throw std::length_error("Count exceeds max size.");

	auto new_size = sz - count + count2;

	auto pop = get_pool();

	transaction::run(pop, [&] {
		if (is_sso_used() && new_size <= sso_capacity) {
			add_sso_to_tx(index, new_size - index + 1);

			assert(count2 < new_size + 1);
			traits_type::move(&sso_data()._data[index + count2],
					  &sso_data()._data[index + count],
					  sz - index - count);
			std::copy(first2, last2, &sso_data()._data[index]);

			set_sso_size(new_size);
			sso_data()._data[new_size] = value_type('\0');
		} else {
			/* 1) Cache C-style string in case of
			 * self-replace, because it will be destroyed
			 * when switching from sso to large string.
			 *
			 * 2) We cache in std::vector instead of
			 * std::string because of overload deduction
			 * ambiguity on Windows
			 */
			std::vector<value_type> str(first2, last2);

			if (is_sso_used()) {
				sso_to_large(new_size);
			}

			auto beg =
				begin() + static_cast<difference_type>(index);
			auto end = beg + static_cast<difference_type>(count);
			non_sso_data().erase(beg, end);
			non_sso_data().insert(beg, str.begin(), str.end());
		}

		if (!is_sso_used() && new_size <= sso_capacity)
			large_to_sso();
	});

	return *this;
}

/**
 * Replace range [first, last) with the characters in [s, s + count2) range
 * transactionally.
 *
 * @param[in] first begin of the range of characters that will be replaced.
 * @param[in] last end of the range of characters that will be replaced.
 * @param[in] s pointer to C-style string that will be used for replacement.
 * @param[in] count2 number of characters that will be used for replacement.
 *
 * @return *this
 *
 * @post size() == size() - std::distance(first, last) + count2.
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(const_iterator first, const_iterator last,
				     const CharT *s, size_type count2)
{
	return replace(first, last, s, s + count2);
}

/**
 * Replace range [index, index + count) with the characters in [s, s + count2)
 * range transactionally.
 *
 * @param[in] index start of the substring that will be replaced.
 * @param[in] count length of the substring that will be replaced.
 * @param[in] s pointer to C-style string that will be used for replacement.
 * @param[in] count2 number of characters that will be used for replacement.
 *
 * @return *this
 *
 * @post size() == size() - (std::min)(count, size() - index) + count2.
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(size_type index, size_type count,
				     const CharT *s, size_type count2)
{
	if (index > size())
		throw std::out_of_range("Index out of range.");

	auto first = cbegin() + static_cast<difference_type>(index);
	auto last = first + static_cast<difference_type>(count);

	return replace(first, last, s, s + count2);
}

/**
 * Replace range [index, index + count) with the characters in [s, s +
 * traits::length(s)) range transactionally.
 *
 * @param[in] index start of the substring that will be replaced.
 * @param[in] count length of the substring that will be replaced.
 * @param[in] s pointer to C-style string that will be used for replacement.
 *
 * @return *this
 *
 * @post size() == size() - (std::min)(count, size() - index) +
 * traits::length(s).
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(size_type index, size_type count,
				     const CharT *s)
{
	return replace(index, count, s, traits_type::length(s));
}

/**
 * Replace range [index, index + count) with count2 copies of ch character
 * transactionally.
 *
 * @param[in] index start of the substring that will be replaced.
 * @param[in] count length of the substring that will be replaced.
 * @param[in] count2 number of characters that will be used for replacement.
 * @param[in] ch character that will be used for replacement.
 *
 * @return *this
 *
 * @post size() == size() - (std::min)(count, size() - index) + count2.
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::out_of_range if index > size().
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(size_type index, size_type count,
				     size_type count2, CharT ch)
{
	if (index > size())
		throw std::out_of_range("Index out of range.");

	auto first = cbegin() + static_cast<difference_type>(index);
	auto last = first + static_cast<difference_type>(count);

	return replace(first, last, count2, ch);
}

/**
 * Replace range [first, last) with count2 copies of ch character
 * transactionally.
 *
 * @param[in] first begin of the range of characters that will be replaced.
 * @param[in] last end of the range of characters that will be replaced.
 * @param[in] count2 number of characters that will be used for replacement.
 * @param[in] ch character that will be used for replacement.
 *
 * @return *this
 *
 * @post size() == size() - std::distance(first, last) + count2.
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(const_iterator first, const_iterator last,
				     size_type count2, CharT ch)
{
	auto sz = size();
	auto index = static_cast<size_type>(std::distance(cbegin(), first));
	auto count = static_cast<size_type>(std::distance(first, last));

	count = (std::min)(count, sz - index);

	if (sz - count + count2 > max_size())
		throw std::length_error("Count exceeds max size.");

	auto new_size = sz - count + count2;

	auto pop = get_pool();

	transaction::run(pop, [&] {
		if (is_sso_used() && new_size <= sso_capacity) {
			add_sso_to_tx(index, new_size - index + 1);

			assert(count2 < new_size + 1);
			traits_type::move(&sso_data()._data[index + count2],
					  &sso_data()._data[index + count],
					  sz - index - count);
			traits_type::assign(&sso_data()._data[index], count2,
					    ch);

			set_sso_size(new_size);
			sso_data()._data[new_size] = value_type('\0');
		} else {
			if (is_sso_used()) {
				sso_to_large(new_size);
			}

			auto beg =
				begin() + static_cast<difference_type>(index);
			auto end = beg + static_cast<difference_type>(count);
			non_sso_data().erase(beg, end);
			non_sso_data().insert(beg, count2, ch);
		}

		if (!is_sso_used() && new_size <= sso_capacity)
			large_to_sso();
	});

	return *this;
}

/**
 * Replace range [first, last) with the characters in [s, s + traits::length(s))
 * range transactionally.
 *
 * @param[in] first begin of the range of characters that will be replaced.
 * @param[in] last end of the range of characters that will be replaced.
 * @param[in] s pointer to C-style string that will be used for replacement.
 *
 * @return *this
 *
 * @post size() == size() - std::distance(first, last) + traits::length(s).
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(const_iterator first, const_iterator last,
				     const CharT *s)
{
	return replace(first, last, s, traits_type::length(s));
}

/**
 * Replace range [first, last) with characters in initializer list ilist
 * transactionally.
 *
 * @param[in] first begin of the range of characters that will be replaced.
 * @param[in] last end of the range of characters that will be replaced.
 * @param[in] ilist initializer list of characters that will be used for
 * replacement.
 *
 * @return *this
 *
 * @post size() == size() - std::distance(first, last) + ilist.size().
 * @post capacity() == sso_capacity if new size is less than or equal to
 * sso_capacity, or the smallest next power of 2, bigger than new size if it is
 * greater than old capacity, or remains the same if there is enough space to
 * store all new elements.
 *
 * @throw std::length_error if new size > max_size().
 * @throw pmem::transaction_alloc_error when allocating new memory failed.
 * @throw pmem::transaction_free_error when freeing old underlying array failed.
 * @throw rethrows constructor exception.
 */
template <typename CharT, typename Traits>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::replace(const_iterator first, const_iterator last,
				     std::initializer_list<CharT> ilist)
{
	return replace(first, last, ilist.begin(), ilist.end());
}

/**
 * Copy [index, index + count) substring of *this to C-style string.
 * If either count == npos or count exceeds size of *this string then substring
 * [index, size()) is used. Resulting C-style string is not null-terminated.
 *
 * @param[in] s pointer to destination C-style string.
 * @param[in] count length of the substring.
 * @param[in] index start of the substring that will be copied.
 *
 * @return number of copied characters.
 *
 * @throw std::out_of_range if index > size().
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::copy(CharT *s, size_type count,
				  size_type index) const
{
	auto sz = size();

	if (index > sz)
		throw std::out_of_range("Index out of range.");

	auto len = (std::min)(count, sz - index);

	traits_type::copy(s, data() + index, len);

	return len;
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
 * Finds the first substring equal str.
 *
 * @param[in] str string to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position of the first character of the found substring or
 * npos if no such substring is found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find(const basic_string &str, size_type pos) const
	noexcept
{
	return find(str.data(), pos, str.size());
}

/**
 * Finds the first substring equal to the range [s, s+count).
 * This range may contain null characters.
 *
 * @param[in] s pointer to the C-style string to search for
 * @param[in] pos position where the search starts from
 * @param[in] count length of the substring to search for
 *
 * @return Position of the first character of the found substring or
 * npos if no such substring is found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find(const CharT *s, size_type pos,
				  size_type count) const
{
	auto sz = size();

	if (pos > sz)
		return npos;

	if (count == 0)
		return pos;

	while (pos + count <= sz) {
		auto found = traits_type::find(cdata() + pos, sz - pos, s[0]);
		if (!found)
			return npos;
		pos = static_cast<size_type>(std::distance(cdata(), found));
		if (traits_type::compare(found, s, count) == 0) {
			return pos;
		}
		++pos;
	}
	return npos;
}

/**
 * Finds the first substring equal to the C-style string pointed to by s.
 * The length of the string is determined by the first null character.
 *
 * @param[in] s pointer to the C-style string to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position of the first character of the found substring or
 * npos if no such substring is found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find(const CharT *s, size_type pos) const
{
	return find(s, pos, traits_type::length(s));
}

/**
 * Finds the first character ch
 *
 * @param[in] ch character to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position of the first character equal to ch, or npos if no such
 * character is found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find(CharT ch, size_type pos) const noexcept
{
	return find(&ch, pos, 1);
}

/**
 * Finds the last substring equal to str.
 * If npos or any value not smaller than size()-1 is passed as pos, whole string
 * will be searched.
 * @param[in] str string to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position (as an offset from start of the string) of the first
 * character of the found substring or npos if no such substring is found
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::rfind(const basic_string &str, size_type pos) const
	noexcept
{
	return rfind(str.cdata(), pos, str.size());
}

/**
 * Finds the last substring equal to the range [s, s+count).
 * This range can include null characters. When pos is specified, the search
 * only includes sequences of characters that begin at or before position pos,
 * ignoring any possible match beginning after pos. If npos or any value not
 * smaller than size()-1 is passed as pos, whole string will be searched.
 *
 * @param[in] s pointer to the C-style string to search for
 * @param[in] pos position where the search starts from
 * @param[in] count length of the substring to search for
 *
 * @return Position (as an offset from start of the string) of the first
 * character of the found substring or npos if no such substring is found. If
 * searching for an empty string retrurn pos, if also pos is greater than the
 * size of the string - it returns size
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::rfind(const CharT *s, size_type pos,
				   size_type count) const
{
	if (count <= size()) {
		pos = (std::min)(size() - count, pos);
		do {
			if (traits_type::compare(cdata() + pos, s, count) == 0)
				return pos;
		} while (pos-- > 0);
	}
	return npos;
}

/**
 * Finds the last substring equal to the C-style string pointed to by s.
 * The length of the string is determined by the first null character
 * If npos or any value not smaller than size()-1 is passed as pos, whole string
 * will be searched.
 *
 * @param[in] s pointer to the C-style string to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position (as an offset from start of the string) of the first
 * character of the found substring or npos if no such substring is found
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::rfind(const CharT *s, size_type pos) const
{
	return rfind(s, pos, traits_type::length(s));
}

/**
 * Finds the last character equal to ch.
 * If npos or any value not smaller than size()-1 is passed as pos, whole string
 * will be searched.
 *
 * @param[in] ch character to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position (as an offset from start of the string) of the first
 * character equal to ch or npos if no such character is found
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::rfind(CharT ch, size_type pos) const noexcept
{
	return rfind(&ch, pos, 1);
}

/**
 * Finds the first character equal to any of the characters in str.
 *
 * @param[in] str string identifying characters to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_first_of(const basic_string &str,
					   size_type pos) const noexcept
{
	return find_first_of(str.cdata(), pos, str.size());
}

/**
 * Finds the first character equal to any of the characters
 * in the range [s, s+count). This range can include null characters.
 *
 * @param[in] s pointer to the C-style string identifying characters to search
 *for
 * @param[in] pos position at which to begin searching
 * @param[in] count length of the C-style string identifying characters to
 *search for
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_first_of(const CharT *s, size_type pos,
					   size_type count) const
{
	size_type first_of = npos;
	for (const CharT *c = s; c != s + count; ++c) {
		size_type found = find(*c, pos);
		if (found != npos && found < first_of)
			first_of = found;
	}
	return first_of;
}

/**
 * Finds the first character equal to any of the characters in the C-style
 * string pointed to by s. The length of the string is determined by the first
 * null character
 *
 * @param[in] s pointer to the C-style string identifying characters to search
 * for
 * @param[in] pos position at which to begin searching
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_first_of(const CharT *s, size_type pos) const
{
	return find_first_of(s, pos, traits_type::length(s));
}

/**
 * Finds the first character equal to ch
 *
 * @param[in] ch character to search for
 * @param[in] pos position at which to begin searching
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_first_of(CharT ch, size_type pos) const
	noexcept
{
	return find(ch, pos);
}

/**
 * Finds the first character equal to none of the characters in str.
 *
 * @param[in] str string identifying characters to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_first_not_of(const basic_string &str,
					       size_type pos) const noexcept
{
	return find_first_not_of(str.cdata(), pos, str.size());
}

/**
 * Finds the first character equal to none of the characters
 * in the range [s, s+count). This range can include null characters.
 *
 * @param[in] s pointer to the C-style string identifying characters to search
 *	for
 * @param[in] pos position at which to begin searching
 * @param[in] count length of the C-style string identifying characters to
 *search for
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_first_not_of(const CharT *s, size_type pos,
					       size_type count) const
{
	if (pos >= size())
		return npos;

	for (auto it = cbegin() + pos; it != cend(); ++it)
		if (!traits_type::find(s, count, *it))
			return static_cast<size_type>(
				std::distance(cbegin(), it));
	return npos;
}

/**
 * Finds the first character equal to none of the characters in the C-style
 *string pointed to by s. The length of the string is determined by the first
 *null character
 *
 * @param[in] s pointer to the C-style string identifying characters to search
 *	for
 * @param[in] pos position at which to begin searching
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_first_not_of(const CharT *s,
					       size_type pos) const
{
	return find_first_not_of(s, pos, traits_type::length(s));
}

/**
 * Finds the first character not equal to ch
 *
 * @param[in] ch character to search for
 * @param[in] pos position at which to begin searching
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_first_not_of(CharT ch, size_type pos) const
	noexcept
{
	return find_first_not_of(&ch, pos, 1);
}

/**
 * Finds the last character equal to any of the characters in str.
 *
 * @param[in] str string identifying characters to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_last_of(const basic_string &str,
					  size_type pos) const noexcept
{
	return find_last_of(str.cdata(), pos, str.size());
}

/**
 * Finds the last character equal to any of the characters
 * in the range [s, s+count). This range can include null characters.
 *
 * @param[in] s pointer to the C-style string identifying characters to search
 *for
 * @param[in] pos position at which to begin searching
 * @param[in] count length of the C-style string identifying characters to
 *search for
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_last_of(const CharT *s, size_type pos,
					  size_type count) const
{
	if (size() == 0 || count == 0)
		return npos;

	bool found = false;
	size_type last_of = 0;
	for (const CharT *c = s; c != s + count; ++c) {
		size_type position = rfind(*c, pos);
		if (position != npos) {
			found = true;
			if (position > last_of)
				last_of = position;
		}
	}
	if (!found)
		return npos;
	return last_of;
}

/**
 * Finds the last character equal to any of the characters in the C-style string
 * pointed to by s. The length of the string is determined by the
 * first null character
 *
 * @param[in] s pointer to the C-style string identifying characters to search
 *	for
 * @param[in] pos position at which to begin searching
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_last_of(const CharT *s, size_type pos) const
{
	return find_last_of(s, pos, traits_type::length(s));
}

/**
 * Finds the last character equal to ch
 *
 * @param[in] ch character to search for
 * @param[in] pos position at which to begin searching
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_last_of(CharT ch, size_type pos) const
	noexcept
{
	return rfind(ch, pos);
}

/**
 * Finds the last character equal to none of the characters in str.
 *
 * @param[in] str string identifying characters to search for
 * @param[in] pos position where the search starts from
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_last_not_of(const basic_string &str,
					      size_type pos) const noexcept
{
	return find_last_not_of(str.cdata(), pos, str.size());
}

/**
 * Finds the last character equal to none of the characters
 * in the range [s, s+count). This range can include null characters.
 *
 * @param[in] s pointer to the C-style string identifying characters to search
 *for
 * @param[in] pos position at which to begin searching
 * @param[in] count length of the C-style string identifying characters to
 *search for
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_last_not_of(const CharT *s, size_type pos,
					      size_type count) const
{
	if (size() > 0) {
		pos = (std::min)(pos, size() - 1);
		do {
			if (!traits_type::find(s, count, *(cdata() + pos)))
				return pos;

		} while (pos-- > 0);
	}
	return npos;
}

/**
 * Finds the last character equal to none of the characters in the C-style
 *string pointed to by s. The length of the string is determined by the first
 *null character
 *
 * @param[in] s pointer to the C-style string identifying characters to search
 *	for
 * @param[in] pos position at which to begin searching
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_last_not_of(const CharT *s,
					      size_type pos) const
{
	return find_last_not_of(s, pos, traits_type::length(s));
}

/**
 * Finds the last character not equal to ch
 *
 * @param[in] ch character to search for
 * @param[in] pos position at which to begin searching
 *
 * @return Position of the found character or npos if no such character is
 * found.
 */
template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::size_type
basic_string<CharT, Traits>::find_last_not_of(CharT ch, size_type pos) const
	noexcept
{
	return find_last_not_of(&ch, pos, 1);
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
	return is_sso_used() ? sso_data().cdata() : non_sso_data().cdata();
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
	return is_sso_used() ? sso_capacity : non_sso_data().capacity() - 1;
}

/**
 * Resize the string to count characters transactionally. If the current
 * size is greater than count, the string is reduced to its first count
 * elements. If the current size is less than count, additional
 * characters of ch value are appended.
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
 * @throw pmem::transaction_free_error when freeing old underlying array
 * failed.
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
			set_sso_size(count);
			sso_data()[count] = value_type('\0');
		} else {
			non_sso_data().resize(count + 1, ch);
			non_sso_data().back() = value_type('\0');
		}
	});
}

/**
 * Resize the string to count characters transactionally. If the current
 * size is greater than count, the string is reduced to its first count
 * elements. If the current size is less than count, additional
 * default-initialized characters are appended.
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
 * @throw pmem::transaction_free_error when freeing old underlying array
 * failed.
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
 * allocated, otherwise the method does nothing. If new_cap is greater
 * than capacity(), all iterators, including the past-the-end iterator,
 * and all references to the elements are invalidated. Otherwise, no
 * iterators or references are invalidated.
 *
 * @param[in] new_cap new capacity.
 *
 * @post capacity() == max(capacity(), capacity_new)
 *
 * @throw rethrows destructor exception.
 * @throw std::length_error if new_cap > max_size().
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_free_error when freeing old underlying array
 * failed.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::reserve(size_type new_cap)
{
	if (new_cap > max_size())
		throw std::length_error("New capacity exceeds max size.");

	if (new_cap < capacity() || new_cap <= sso_capacity)
		return;

	if (is_sso_used()) {
		auto pop = get_pool();

		transaction::run(pop, [&] { sso_to_large(new_cap); });
	} else {
		non_sso_data().reserve(new_cap + 1);
	}
}

/**
 * Remove unused capacity transactionally. If large string is used
 * capacity will be set to current size. If sso is used nothing happens.
 *
 * @post capacity() == std::min(sso_capacity, capacity())
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when reallocating failed.
 * @throw pmem::transaction_free_error when freeing old underlying array
 * failed.
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
		auto pop = get_pool();

		transaction::run(pop, [&] { large_to_sso(); });
	} else {
		non_sso_data().shrink_to_fit();
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
	erase(begin(), end());
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
	return (sso._size & _sso_mask) != 0;
}

template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::destroy_data()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	if (is_sso_used()) {
		add_sso_to_tx(0, get_sso_size() + 1);
		/* sso.data destructor does not have to be called */
	} else {
		non_sso_data().free_data();
		detail::destroy<non_sso_type>(non_sso_data());
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
 * Generic function which replace_content current content based on provided
 * parameters. Allowed parameters are:
 * - size_type count, CharT value
 * - InputIt first, InputIt last
 * - basic_string &&
 */
template <typename CharT, typename Traits>
template <typename... Args>
typename basic_string<CharT, Traits>::pointer
basic_string<CharT, Traits>::replace_content(Args &&... args)
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
		auto ptr = assign_sso_data(std::forward<Args>(args)...);
		set_sso_size(size);

		return ptr;
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
		detail::conditional_add_to_tx(&non_sso_data(), 1,
					      POBJ_XADD_NO_SNAPSHOT);
		detail::create<non_sso_type>(&non_sso_data());
		non_sso_data().reserve(capacity + 1);
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

	add_sso_to_tx(0, size + 1);
	std::copy(first, last, &sso_data()._data[0]);

	sso_data()._data[size] = value_type('\0');

	return &sso_data()[0];
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

	add_sso_to_tx(0, count + 1);
	traits_type::assign(&sso_data()._data[0], count, ch);

	sso_data()._data[count] = value_type('\0');

	return &sso_data()[0];
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

	non_sso_data().reserve(size + 1);
	non_sso_data().assign(first, last);
	non_sso_data().push_back(value_type('\0'));

	return non_sso_data().data();
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

	non_sso_data().reserve(count + 1);
	non_sso_data().assign(count, ch);
	non_sso_data().push_back(value_type('\0'));

	return non_sso_data().data();
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

	non_sso_data() = std::move(other.non_sso_data());

	return non_sso_data().data();
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
		throw pmem::pool_error("Object is not on pmem.");
}

/**
 * @throw pmem::transaction_scope_error if called outside of a transaction.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::check_tx_stage_work() const
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"Call made out of transaction scope.");
}

/**
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if called outside of a transaction.
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
basic_string<CharT, Traits>::add_sso_to_tx(size_type idx_first,
					   size_type num) const
{
	assert(idx_first + num <= sso_capacity + 1);
	assert(is_sso_used());

	auto initialized_num = get_sso_size() + 1 - idx_first;

	/* Snapshot elements in range [idx_first, sso_size + 1 (null)) */
	detail::conditional_add_to_tx(&sso_data()._data[0] + idx_first,
				      (std::min)(initialized_num, num));

	if (num > initialized_num) {
		/* Elements after sso_size + 1 do not have to be snapshotted */
		detail::conditional_add_to_tx(
			&sso_data()._data[0] + get_sso_size() + 1,
			num - initialized_num, POBJ_XADD_NO_SNAPSHOT);
	}
}

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
 * Resize sso string to large string.
 * Capacity is equal new_capacity plus sizeof(CharT) bytes for null character.
 * Content of sso string is preserved and copied to the large string object.
 *
 * @pre must be called in transaction scope.
 *
 * @post sso is not used.
 *
 * @param[in] new_capacity capacity of constructed large string.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::sso_to_large(size_t new_capacity)
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(new_capacity > sso_capacity);
	assert(is_sso_used());

	auto sz = size();

	sso_type tmp;
	std::copy(cbegin(), cend(), tmp.data());
	tmp[sz] = value_type('\0');

	destroy_data();
	allocate(new_capacity);

	auto begin = tmp.cbegin();
	auto end = begin + sz;

	initialize(begin, end);

	assert(!is_sso_used());
};

/**
 * Resize large string to sso string of size() size.
 * Content of large string is preserved and copied to the sso string.
 *
 * @pre must be called in transaction scope.
 * @pre size() of large string must be less than or equal sso_capacity
 *
 * @post sso is used.
 */
template <typename CharT, typename Traits>
void
basic_string<CharT, Traits>::large_to_sso()
{
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);
	assert(!is_sso_used());

	auto sz = size();

	assert(sz <= sso_capacity);

	sso_type tmp;
	std::copy(cbegin(), cbegin() + sz, tmp.data());
	tmp[sz] = value_type('\0');

	destroy_data();
	allocate(sz);

	auto begin = tmp.cbegin();
	auto end = begin + sz;

	initialize(begin, end);

	assert(is_sso_used());
};

template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::non_sso_type &
basic_string<CharT, Traits>::non_sso_data()
{
	assert(!is_sso_used());
	return non_sso._data;
}

template <typename CharT, typename Traits>
typename basic_string<CharT, Traits>::sso_type &
basic_string<CharT, Traits>::sso_data()
{
	assert(is_sso_used());
	return sso._data;
}

template <typename CharT, typename Traits>
const typename basic_string<CharT, Traits>::non_sso_type &
basic_string<CharT, Traits>::non_sso_data() const
{
	assert(!is_sso_used());
	return non_sso._data;
}

template <typename CharT, typename Traits>
const typename basic_string<CharT, Traits>::sso_type &
basic_string<CharT, Traits>::sso_data() const
{
	assert(is_sso_used());
	return sso._data;
}

/**
 * Participate in overload resolution only if T is convertible to size_type.
 * Call basic_string &erase(size_type index, size_type count = npos) if enabled.
 */
template <typename CharT, typename Traits>
template <typename T, typename Enable>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::erase(T param)
{
	return erase(static_cast<size_type>(param));
}

/**
 * Participate in overload resolution only if T is not convertible to size_type.
 * Call iterator erase(const_iterator pos) if enabled.
 */
template <typename CharT, typename Traits>
template <typename T, typename Enable>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::erase(T param)
{
	return erase(static_cast<const_iterator>(param));
}

/**
 * Participate in overload resolution only if T is convertible to size_type.
 * Call basic_string &insert(size_type index, size_type count, CharT ch) if
 * enabled.
 */
template <typename CharT, typename Traits>
template <typename T, typename Enable>
basic_string<CharT, Traits> &
basic_string<CharT, Traits>::insert(T param, size_type count, CharT ch)
{
	return insert(static_cast<size_type>(param), count, ch);
}

/**
 * Participate in overload resolution only if T is not convertible to size_type.
 * Call iterator insert(const_iterator pos, size_type count, CharT ch) if
 * enabled.
 */
template <typename CharT, typename Traits>
template <typename T, typename Enable>
typename basic_string<CharT, Traits>::iterator
basic_string<CharT, Traits>::insert(T param, size_type count, CharT ch)
{
	return insert(static_cast<const_iterator>(param), count, ch);
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

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_BASIC_STRING_HPP */
