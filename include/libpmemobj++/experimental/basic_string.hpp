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
 * String container with std::basic_string compatible interface.
 */

#ifndef LIBPMEMOBJ_CPP_BASIC_STRING_HPP
#define LIBPMEMOBJ_CPP_BASIC_STRING_HPP

#include <algorithm>
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
	using const_iterator = const_contiguous_iterator<CharT>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	/* Number of characters which can be stored using sso */
	static constexpr size_type sso_capacity = 64 - sizeof('\0');

	/**
	 * Default constructor. Constructs an empty container.
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

		_size = 0;
	}

	/**
	 * Constructs the container with count copies of elements with value ch.
	 *
	 * @param[in] count number of elements to construct.
	 * @param[in] value value of all constructed elements.
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
	 * Constructs the string with a substring [pos, pos+count) of other. If
	 * count == npos, if count is not specified, or if the requested
	 * substring lasts past the end of the string, the resulting substring
	 * is [pos, other.size()).
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
	 * Constructs the string with the first count characters of character
	 * string pointed to by s. s can contain null characters. The length of
	 * the string is count. The behavior is undefined if [s, s + count) is
	 * not a valid range.
	 *
	 * @param[in] s pointer to source string.
	 * @param[in] count length of the string.
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
	 * Constructs the string with the contents initialized with a copy of
	 * the null-terminated character string pointed to by s.
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
	 * Constructs the string with the contents of the range [first, last).
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
		check_pmem_tx();

		initialize(first, last);
	}

	/**
	 * Copy constructor. Constructs the string with the copy of the contents
	 * of other.
	 *
	 * @param[in] other reference to the vector to be copied.
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
	 * Move constructor. Constructs the string with the contents of other
	 * using move semantics. other is left in valid, but unspecified state.
	 *
	 * @param[in] other rvalue reference to the vector to be moved from.
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

		/*
		 * XXX: enable move assignment when vector is ready.
		 * if (!other.uses_sso())
		 * data_large = std::move(other.data_large);
		 * else
		 */
		initialize(other.cbegin(), other.cend());

		if (!other.uses_sso())
			other.data_large.clear();

		other._size = 0;
	}

	/**
	 * Constructs the container with the contents of the initializer list
	 * init.
	 *
	 * @param[in] init initializer list with content to be constructed.
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
	 */
	~basic_string()
	{
		if (!uses_sso())
			detail::destroy<decltype(data_large)>(data_large);
	}

	/**
	 * Returns an iterator to the beginning.
	 *
	 * @return an iterator pointing to the first element in the string.
	 */
	iterator
	begin()
	{
		if (uses_sso())
			return iterator(data_sso.begin().get_ptr());
		else
			return iterator(data_large.begin().get_ptr());
	}

	/**
	 * Returns const iterator to the beginning.
	 *
	 * @return const iterator pointing to the first element in the string.
	 */
	const_iterator
	begin() const noexcept
	{
		return cbegin();
	}

	/**
	 * Returns const iterator to the beginning.
	 *
	 * @return const iterator pointing to the first element in the string.
	 */
	const_iterator
	cbegin() const noexcept
	{
		if (uses_sso())
			return const_iterator(data_sso.cbegin().get_ptr());
		else
			return const_iterator(data_large.cbegin().get_ptr());
	}

	/**
	 * Returns an iterator to past the end.
	 *
	 * @return iterator referring to the past-the-end element in the string.
	 */
	iterator
	end()
	{
		return begin() + static_cast<difference_type>(size());
	}

	/**
	 * Returns const iterator to past the end.
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
	 * Returns const iterator to past the end.
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
	 * Access element at specific index with bounds checking and snapshot it
	 * if there is an active transaction.
	 *
	 * @param[in] n index number.
	 *
	 * @return reference to element number n in underlying array.
	 *
	 * @pre must be called in transaction scope.
	 *
	 * @throw std::out_of_range if n is not within the range of the
	 * container.
	 * @throw pmem::transaction_error when adding the object to the
	 * transaction failed.
	 */
	reference
	at(size_type n)
	{
		return uses_sso() ? data_sso.at(n) : data_large.at(n);
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
		return uses_sso() ? data_sso.const_at(n)
				  : data_large.const_at(n);
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
		return uses_sso() ? data_sso[n] : data_large[n];
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
		return uses_sso() ? data_sso[n] : data_large[n];
	}

	/**
	 * @return number of CharT elements in the string.
	 */
	size_type
	size() const noexcept
	{
		return _size.get_ro();
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
		if (uses_sso())
			return data_sso.range(0, size() + sizeof('\0')).begin();
		else
			return data_large.data();
	}

	/**
	 * @return const pointer to underlying data.
	 */
	const CharT *
	cdata() const noexcept
	{
		return uses_sso() ? data_sso.cdata() : data_large.cdata();
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
		return _size.get_ro();
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
		if (uses_sso())
			return sso_capacity;
		else
			return data_large.capacity() - sizeof('\0');
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
	/*
	 * This union holds sso data inside of an array and non sso data inside
	 * a vector. If vector is used, it must be manually created and
	 * destroyed.
	 */
	union {
		array<value_type, sso_capacity + sizeof('\0')> data_sso;

		vector<value_type> data_large;
	};

	/* Number of characters stored (excluding null-terminator) */
	p<size_type> _size;

	/**
	 * Checks if sso would be used for size equal to s.
	 */
	bool
	uses_sso(size_type s) const noexcept
	{
		return s <= sso_capacity;
	}

	/**
	 * Checks if sso is currently used.
	 */
	bool
	uses_sso() const noexcept
	{
		return size() <= sso_capacity;
	}

	/**
	 * Returns std::distance(first, last).
	 */
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	size_type
	get_size(InputIt first, InputIt last)
	{
		return static_cast<size_type>(std::distance(first, last));
	}

	/**
	 * Returns new_size.
	 */
	size_type
	get_size(size_type new_size, value_type ch)
	{
		return new_size;
	}

	/**
	 * Generic function which initializes memory based on parameters
	 * provided. Allowed parameters are:
	 * - size_type count, CharT value
	 * - InputIt first, InputIt last
	 *
	 * @pre must be called in transaction scope.
	 */
	template <typename... Args>
	pointer
	initialize(Args... args)
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		_size = get_size(args...);

		if (uses_sso())
			return init_sso_data(args...);
		else
			return init_large_data(args...);
	}

	/**
	 * Initialize sso data. Overload for pair of iterators
	 */
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	pointer
	init_sso_data(InputIt first, InputIt last)
	{
		assert(size() <= sso_capacity);

		/*
		 * array is aggregate type so it's not required to call a
		 * contructor.
		 */
		auto dst = data_sso.data();
		std::copy(first, last, dst);

		dst[size()] = '\0';

		return dst;
	}

	/**
	 * Initialize sso data. Overload for character.
	 */
	pointer
	init_sso_data(size_type new_size, value_type ch)
	{
		assert(size() <= sso_capacity);

		/*
		 * array is aggregate type so it's not required to call a
		 * contructor.
		 */
		auto dst = data_sso.data();
		traits_type::assign(dst, size(), ch);

		dst[size()] = '\0';

		return dst;
	}

	/**
	 * Initialize data_large - call constructor of data_large.
	 */
	template <
		typename InputIt,
		typename Enable = typename std::enable_if<
			pmem::detail::is_input_iterator<InputIt>::value>::type>
	pointer
	init_large_data(InputIt first, InputIt last)
	{
		assert(size() > sso_capacity);

		detail::create<decltype(data_large)>(&data_large,
						     size() + sizeof('\0'));

		auto dst = data_large.data();
		std::copy(first, last, dst);
		dst[size()] = '\0';

		// XXX: Enable once vector provides insert and push back.
		// This will allow to delete second init_large_data overload.
		// detail::create<decltype(data_large)>(&data_large);
		// data_large.reserve(size() + sizeof('\0'));
		// data_large.insert(0, first, last);
		// data_large.push_back('\0');

		return dst;
	}

	/**
	 * Initialize data_large - call constructor of data_large.
	 */
	pointer
	init_large_data(size_type new_size, value_type ch)
	{
		assert(new_size > sso_capacity);

		detail::create<decltype(data_large)>(
			&data_large, new_size + sizeof('\0'), ch);
		data_large[new_size] = '\0';

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
			throw pool_error("Invalid pool handle.");
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

} /* namespace experimental */

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_BASIC_STRING_HPP */
