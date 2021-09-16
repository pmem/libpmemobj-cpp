// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

/**
 * @file
 * Inline string implementation.
 */

#ifndef LIBPMEMOBJ_CPP_INLINE_STRING_HPP
#define LIBPMEMOBJ_CPP_INLINE_STRING_HPP

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

#include <string>

namespace pmem
{

namespace obj
{

namespace experimental
{

template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_inline_string_base {
public:
	using traits_type = Traits;
	using value_type = CharT;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	basic_inline_string_base(basic_string_view<CharT, Traits> v);
	basic_inline_string_base(size_type capacity);
	basic_inline_string_base(const basic_inline_string_base &rhs);

	basic_inline_string_base &
	operator=(const basic_inline_string_base &rhs);

	basic_inline_string_base &
	operator=(basic_string_view<CharT, Traits> rhs);

	basic_inline_string_base(basic_inline_string_base<CharT> &&) = delete;

	basic_inline_string_base &
	operator=(basic_inline_string_base &&) = delete;
	operator basic_string_view<CharT, Traits>() const;

	size_type size() const noexcept;
	size_type capacity() const noexcept;

	pointer data();
	const_pointer data() const noexcept;
	const_pointer cdata() const noexcept;

	int compare(basic_string_view<CharT, Traits> rhs) const noexcept;

	reference operator[](size_type p);
	const_reference operator[](size_type p) const noexcept;

	reference at(size_type p);
	const_reference at(size_type p) const;

	slice<pointer> range(size_type p, size_type count);

	basic_inline_string_base &assign(basic_string_view<CharT, Traits> rhs);

protected:
	pointer snapshotted_data(size_t p, size_t n);

	obj::p<uint64_t> size_;
	obj::p<uint64_t> capacity_;
};

/**
 * Pmem/DRAM variation of pmem::obj::string, where data is kept right next
 * to the inline_string structure.
 *
 * @note Unlike other containers, it can be used on pmem and dram. Modifiers
 * (like assign()) can only be called if inline string is kept on pmem, though.
 *
 * pmem::obj::experimental::basic_dram_inline_string serves similar purpose to
 * pmem::obj::string, but keeps the data within the same allocation
 * as inline_string itself. It means, the data is always kept right
 * after the inline_string structure.
 *
 * Creating an object of inline_string must be hence done as follows:
 * 1. Allocate memory of sizeof(inline_string) + size of the characters string +
 * sizeof('\0')
 * 2. Use emplace new() to create inline_string
 *
 * Example:
 * @snippet inline_string/inline_string.cpp inline_string_example
 * @ingroup experimental_containers
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_dram_inline_string
    : public basic_inline_string_base<CharT, Traits> {
public:
	using traits_type = Traits;
	using value_type = CharT;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using basic_inline_string_base<CharT, Traits>::operator=;

	basic_dram_inline_string(basic_string_view<CharT, Traits> v)
	    : basic_inline_string_base<CharT, Traits>(v)
	{
	}

	basic_dram_inline_string(size_type capacity)
	    : basic_inline_string_base<CharT, Traits>(capacity)
	{
	}
	basic_dram_inline_string(const basic_dram_inline_string &rhs)
	    : basic_inline_string_base<CharT, Traits>(rhs)
	{
	}

	basic_dram_inline_string &
	operator=(const basic_dram_inline_string &rhs)
	{
		return static_cast<basic_dram_inline_string &>(this->operator=(
			static_cast<const basic_inline_string_base<
				CharT, Traits> &>(rhs)));
	}
};

/**
 * Pmem-only variation of pmem::obj::string, where data is kept right next
 * to the inline_string structure.
 *
 * @note It can be kept only on pmem.
 *
 * pmem::obj::experimental::basic_inline_string serves similar purpose to
 * pmem::obj::string, but keeps the data within the same allocation
 * as inline_string itself. It means, the data is always kept right
 * after the inline_string structure.
 *
 * Creating an object of inline_string must be hence done as follows:
 * 1. Allocate memory of sizeof(inline_string) + size of the characters string +
 * sizeof('\0')
 * 2. Use emplace new() to create inline_string
 *
 * Example:
 * @snippet inline_string/inline_string.cpp inline_string_example
 * @ingroup experimental_containers
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_inline_string : public basic_inline_string_base<CharT, Traits> {
public:
	using traits_type = Traits;
	using value_type = CharT;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using basic_inline_string_base<CharT, Traits>::operator=;

	/**
	 * @throw pool_error if inline string is not on pmem.
	 */
	basic_inline_string(basic_string_view<CharT, Traits> v)
	    : basic_inline_string_base<CharT, Traits>(check_forward(v))
	{
	}

	/**
	 * @throw pool_error if inline string is not on pmem.
	 */
	basic_inline_string(size_type capacity)
	    : basic_inline_string_base<CharT, Traits>(check_forward(capacity))
	{
	}

	/**
	 * @throw pool_error if inline string is not on pmem.
	 */
	basic_inline_string(const basic_inline_string &rhs)
	    : basic_inline_string_base<CharT, Traits>(check_forward(rhs))
	{
	}

	basic_inline_string &
	operator=(const basic_inline_string &rhs)
	{
		return static_cast<basic_inline_string &>(this->operator=(
			static_cast<const basic_inline_string_base<
				CharT, Traits> &>(rhs)));
	}

private:
	template <typename T>
	T &&
	check_forward(T &&t)
	{
		if (nullptr == pmemobj_pool_by_ptr(this))
			throw pmem::pool_error("Invalid pool handle.");
		return std::forward<T>(t);
	}
};

using dram_inline_string = basic_dram_inline_string<char>;
using dram_inline_wstring = basic_dram_inline_string<wchar_t>;
using dram_inline_u16string = basic_dram_inline_string<char16_t>;
using dram_inline_u32string = basic_dram_inline_string<char32_t>;

using inline_string = basic_inline_string<char>;
using inline_wstring = basic_inline_string<wchar_t>;
using inline_u16string = basic_inline_string<char16_t>;
using inline_u32string = basic_inline_string<char32_t>;

/**
 * Constructs inline string from a string_view.
 */
template <typename CharT, typename Traits>
basic_inline_string_base<CharT, Traits>::basic_inline_string_base(
	basic_string_view<CharT, Traits> v)
    : size_(v.size()), capacity_(v.size())
{
	std::copy(v.data(), v.data() + static_cast<ptrdiff_t>(size_), data());

	data()[static_cast<ptrdiff_t>(size_)] = '\0';
}

/**
 * Constructs empty inline_string with specified capacity.
 */
template <typename CharT, typename Traits>
basic_inline_string_base<CharT, Traits>::basic_inline_string_base(
	size_type capacity)
    : size_(0), capacity_(capacity)
{
	data()[static_cast<ptrdiff_t>(size_)] = '\0';
}

/**
 * Copy constructor
 */
template <typename CharT, typename Traits>
basic_inline_string_base<CharT, Traits>::basic_inline_string_base(
	const basic_inline_string_base &rhs)
    : size_(rhs.size()), capacity_(rhs.capacity())
{
	std::copy(rhs.data(), rhs.data() + static_cast<ptrdiff_t>(size_),
		  data());

	data()[static_cast<ptrdiff_t>(size_)] = '\0';
}

/**
 * Copy assignment operator
 */
template <typename CharT, typename Traits>
basic_inline_string_base<CharT, Traits> &
basic_inline_string_base<CharT, Traits>::operator=(
	const basic_inline_string_base &rhs)
{
	if (this == &rhs)
		return *this;

	return assign(rhs);
}

/**
 * Assignment operator from string_view.
 */
template <typename CharT, typename Traits>
basic_inline_string_base<CharT, Traits> &
basic_inline_string_base<CharT, Traits>::operator=(
	basic_string_view<CharT, Traits> rhs)
{
	return assign(rhs);
}

/** Conversion operator to string_view */
template <typename CharT, typename Traits>
basic_inline_string_base<CharT, Traits>::operator basic_string_view<
	CharT, Traits>() const
{
	return {data(), size()};
}

/** @return size of the string */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::size_type
basic_inline_string_base<CharT, Traits>::size() const noexcept
{
	return size_;
}

/**
 * @return number of characters that can be held in the inline_string.
 *
 * The space actually occupied by inline_string is equal to
 * sizeof(inline_string) + capacity() + sizeof('\0') and cannot be
 * expanded.
 */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::size_type
basic_inline_string_base<CharT, Traits>::capacity() const noexcept
{
	return capacity_;
}

/**
 * Returns pointer to the underlying data and if
 * there is an active transaction add entire data to a
 * transaction.
 *
 * @return pointer to the data (equal to (this + 1))
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::pointer
basic_inline_string_base<CharT, Traits>::data()
{
	return snapshotted_data(0, size_);
}

/** @return const_pointer to the data (equal to (this + 1)) */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::const_pointer
basic_inline_string_base<CharT, Traits>::data() const noexcept
{
	return cdata();
}

/**
 * Returns const pointer to the underlying data. In contradiction to data(),
 * cdata() will return const_pointer not depending on the const-qualification of
 * the object it is called on.
 *
 * @return const_pointer to the data (equal to (this + 1))
 */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::const_pointer
basic_inline_string_base<CharT, Traits>::cdata() const noexcept
{
	return reinterpret_cast<const CharT *>(this + 1);
}

/**
 * Compares this inline_string with other. Works in the same way as
 * std::basic_string::compare.
 *
 * @return 0 if both character sequences compare equal,
 *			positive value if this is lexicographically greater than
 * other, negative value if this is lexicographically less than other.
 */
template <typename CharT, typename Traits>
int
basic_inline_string_base<CharT, Traits>::compare(
	basic_string_view<CharT, Traits> rhs) const noexcept
{
	return basic_string_view<CharT, Traits>(data(), size()).compare(rhs);
}

/**
 * Returns reference to a character at position @param[in] p and snapshot it if
 * there is an active transaction. No bounds checking is performed.
 *
 * @return reference to a CharT
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::reference
	basic_inline_string_base<CharT, Traits>::operator[](size_type p)
{
	return snapshotted_data(p, 1)[0];
}

/**
 * Returns reference to a character at position @param[in] p
 * No bounds checking is performed.
 *
 * @return const_reference to a CharT
 */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::const_reference
	basic_inline_string_base<CharT, Traits>::operator[](size_type p) const
	noexcept
{
	return cdata()[p];
}

/**
 * Returns reference to a character at position @param[in] p with bounds
 * checking and snapshot it if there is an active transaction.
 *
 * @return reference to a CharT
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw std::out_of_range if p is not within the range of the container.
 */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::reference
basic_inline_string_base<CharT, Traits>::at(size_type p)
{
	if (p >= size())
		throw std::out_of_range("basic_inline_string_base::at");

	return snapshotted_data(p, 1)[0];
}

/**
 * Returns reference to a character at position @param[in] p with bounds
 * checking.
 *
 * @return const_reference to a CharT
 *
 * @throw std::out_of_range if p is not within the range of the container.
 */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::const_reference
basic_inline_string_base<CharT, Traits>::at(size_type p) const
{
	if (p >= size())
		throw std::out_of_range("basic_inline_string_base::at");

	return cdata()[p];
}

/**
 * Returns slice and snapshots (if there is an active transaction) requested
 * range. This method is not specified by STL standards.
 *
 * @param[in] start start index of requested range.
 * @param[in] n number of elements in range.
 *
 * @return slice from start to start + n.
 *
 * @throw std::out_of_range if any element of the range would be outside of the
 * container.
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename CharT, typename Traits>
slice<typename basic_inline_string_base<CharT, Traits>::pointer>
basic_inline_string_base<CharT, Traits>::range(size_type start, size_type n)
{
	if (start + n > size())
		throw std::out_of_range("basic_inline_string_base::range");

	auto data = snapshotted_data(start, n);

	return {data, data + n};
}

/**
 * Return pointer to data at position p and if there is an active transaction
 * snapshot elements from p to p + n.
 */
template <typename CharT, typename Traits>
typename basic_inline_string_base<CharT, Traits>::pointer
basic_inline_string_base<CharT, Traits>::snapshotted_data(size_t p, size_t n)
{
	assert(p + n <= size());

	detail::conditional_add_to_tx(reinterpret_cast<CharT *>(this + 1) + p,
				      n, POBJ_XADD_ASSUME_INITIALIZED);

	return reinterpret_cast<CharT *>(this + 1) + p;
}

/**
 * Transactionally assign content of basic_string_view.
 *
 * @throw std::out_of_range if rhs is larger than capacity.
 * @throw pool_error if inline string is not on pmem.
 */
template <typename CharT, typename Traits>
basic_inline_string_base<CharT, Traits> &
basic_inline_string_base<CharT, Traits>::assign(
	basic_string_view<CharT, Traits> rhs)
{
	auto cpop = pmemobj_pool_by_ptr(this);
	if (nullptr == cpop)
		throw pmem::pool_error("Invalid pool handle.");

	auto pop = pool_base(cpop);

	if (rhs.size() > capacity())
		throw std::out_of_range("inline_string capacity exceeded.");

	auto initialized_mem =
		(std::min)(rhs.size(), size()) + 1 /* sizeof('\0') */;

	obj::flat_transaction::run(pop, [&] {
		detail::conditional_add_to_tx(data(), initialized_mem);

		if (rhs.size() > size())
			detail::conditional_add_to_tx(
				data() + initialized_mem,
				rhs.size() - initialized_mem + 1,
				POBJ_XADD_NO_SNAPSHOT);

		std::copy(rhs.data(),
			  rhs.data() + static_cast<ptrdiff_t>(rhs.size()),
			  data());
		size_ = rhs.size();

		data()[static_cast<ptrdiff_t>(size_)] = '\0';
	});

	return *this;
}

/**
 * A helper trait which calculates required memory capacity (in bytes) for a
 * type.
 *
 * All standard types require capacity equal to the sizeof of such type.
 */
template <typename T>
struct total_sizeof {
	template <typename... Args>
	static size_t
	value(const Args &... args)
	{
		return sizeof(T);
	}
};

/**
 * A helper trait which calculates required memory capacity (in bytes) for a
 * type.
 *
 * Inline_string requires capacity of sizeof(basic_inline_string<CharT>) + size
 * of the data itself.
 */
template <typename CharT, typename Traits>
struct total_sizeof<basic_inline_string<CharT, Traits>> {
	static size_t
	value(const basic_string_view<CharT, Traits> &s)
	{
		return sizeof(basic_inline_string_base<CharT, Traits>) +
			(s.size() + 1 /* '\0' */) * sizeof(CharT);
	}
};

/**
 * A helper trait which calculates required memory capacity (in bytes) for a
 * type.
 *
 * Inline_string requires capacity of sizeof(basic_dram_inline_string<CharT>) +
 * size of the data itself.
 */
template <typename CharT, typename Traits>
struct total_sizeof<basic_dram_inline_string<CharT, Traits>> {
	static size_t
	value(const basic_string_view<CharT, Traits> &s)
	{
		return sizeof(basic_dram_inline_string<CharT, Traits>) +
			(s.size() + 1 /* '\0' */) * sizeof(CharT);
	}
};
} /* namespace experimental */
} /* namespace obj */

namespace detail
{
/* Check if type is pmem::obj::basic_inline_string or
 * pmem::obj::basic_dram_inline_string */
template <typename>
struct is_inline_string : std::false_type {
};

template <typename CharT, typename Traits>
struct is_inline_string<obj::experimental::basic_inline_string<CharT, Traits>>
    : std::true_type {
};

template <typename CharT, typename Traits>
struct is_inline_string<
	obj::experimental::basic_dram_inline_string<CharT, Traits>>
    : std::true_type {
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_INLINE_STRING_HPP */
