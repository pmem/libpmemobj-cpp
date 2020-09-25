// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * Our partial std::string_view implementation.
 */

#ifndef LIBPMEMOBJ_CPP_STRING_VIEW
#define LIBPMEMOBJ_CPP_STRING_VIEW

#include <algorithm>
#include <string>

#if __cpp_lib_string_view
#include <string_view>
#endif

namespace pmem
{

namespace obj
{

#if __cpp_lib_string_view

template <typename CharT, typename Traits = std::char_traits<CharT>>
using basic_string_view = std::basic_string_view<CharT, Traits>;
using string_view = std::string_view;
using wstring_view = std::basic_string_view<wchar_t>;
using u16string_view = std::basic_string_view<char16_t>;
using u32string_view = std::basic_string_view<char32_t>;

#else

/**
 * Our partial std::string_view implementation.
 *
 * If C++17's std::string_view implementation is not available, this one is
 * used to avoid unnecessary string copying.
 */
template <typename CharT, typename Traits = std::char_traits<CharT>>
class basic_string_view {
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

	basic_string_view() noexcept;
	basic_string_view(const CharT *data, size_type size);
	basic_string_view(const std::basic_string<CharT, Traits> &s);
	basic_string_view(const CharT *data);

	basic_string_view(const basic_string_view &rhs) noexcept = default;
	basic_string_view &
	operator=(const basic_string_view &rhs) noexcept = default;

	const CharT *data() const noexcept;
	size_type size() const noexcept;

	const CharT &operator[](size_type p) const noexcept;

	int compare(const basic_string_view &other) noexcept;

private:
	const value_type *data_;
	size_type size_;
};

using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
using u16string_view = basic_string_view<char16_t>;
using u32string_view = basic_string_view<char32_t>;
/**
 * Default constructor with empty data.
 */
template <typename CharT, typename Traits>
inline basic_string_view<CharT, Traits>::basic_string_view() noexcept
    : data_(nullptr), size_(0)
{
}

/**
 * Constructor initialized by *data* and its *size*.
 *
 * @param[in] data pointer to the C-like string to initialize with,
 *	it can contain null characters.
 * @param[in] size length of the given data.
 */
template <typename CharT, typename Traits>
inline basic_string_view<CharT, Traits>::basic_string_view(const CharT *data,
							   size_type size)
    : data_(data), size_(size)
{
}

/**
 * Constructor initialized by the basic string *s*.
 *
 * @param[in] s reference to the string to initialize with.
 */
template <typename CharT, typename Traits>
inline basic_string_view<CharT, Traits>::basic_string_view(
	const std::basic_string<CharT, Traits> &s)
    : data_(s.c_str()), size_(s.size())
{
}

/**
 * Constructor initialized by *data*. Size of the data will be set
 * using Traits::length().
 *
 * @param[in] data pointer to C-like string (char *) to initialize with,
 *	it has to end with the terminating null character.
 */
template <typename CharT, typename Traits>
inline basic_string_view<CharT, Traits>::basic_string_view(const CharT *data)
    : data_(data), size_(Traits::length(data))
{
}

/**
 * Returns pointer to data stored in this pmem::obj::string_view. It may not
 * contain the terminating null character.
 *
 * @return pointer to C-like string (char *), it may not end with null
 *	character.
 */
template <typename CharT, typename Traits>
inline const CharT *
basic_string_view<CharT, Traits>::data() const noexcept
{
	return data_;
}

/**
 * Returns count of characters stored in this pmem::obj::string_view data.
 *
 * @return pointer to C-like string (char *), it may not end with null
 *	character.
 */
template <typename CharT, typename Traits>
inline typename basic_string_view<CharT, Traits>::size_type
basic_string_view<CharT, Traits>::size() const noexcept
{
	return size_;
}

/**
 * Returns reference to a character at position @param[in] p .
 *
 * @return reference to a char.
 */
template <typename CharT, typename Traits>
inline const CharT &basic_string_view<CharT, Traits>::operator[](size_t p) const
	noexcept
{
	return data()[p];
}

/**
 * Compares this string_view with other. Works in the same way as
 * std::basic_string::compare.
 *
 * @return 0 if both character sequences compare equal,
 *	positive value if this is lexicographically greater than other,
 *	negative value if this is lexicographically less than other.
 */
template <typename CharT, typename Traits>
inline int
basic_string_view<CharT, Traits>::compare(
	const basic_string_view &other) noexcept
{
	int ret = Traits::compare(data(), other.data(),
				  (std::min)(size(), other.size()));
	if (ret != 0)
		return ret;
	if (size() < other.size())
		return -1;
	if (size() > other.size())
		return 1;
	return 0;
}
#endif

} /* namespace obj */
} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_STRING_VIEW */
