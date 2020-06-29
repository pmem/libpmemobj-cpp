// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

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
using string_view = std::string_view;
#else
/* XXX: implement it as a template (basic_string_view) */

/*! \class string_view
	\brief Our brief std::string_view implementation.

	If C++17's std::string_view implementation is not available, this one is
   used to avoid unnecessary string copying.
*/
class string_view {
public:
	string_view() noexcept;
	string_view(const char *data, size_t size);
	string_view(const std::string &s);
	string_view(const char *data);

	string_view(const string_view &rhs) noexcept = default;
	string_view &operator=(const string_view &rhs) noexcept = default;

	const char *data() const noexcept;
	std::size_t size() const noexcept;

	const char &operator[](size_t p) const noexcept;

	int compare(const string_view &other) noexcept;

private:
	const char *_data;
	std::size_t _size;
};

/**
 * Default constructor with empty data.
 */
inline string_view::string_view() noexcept : _data(""), _size(0)
{
}

/**
 * Constructor initialized by *data* and its *size*.
 *
 * @param[in] data pointer to the C-like string (char *) to initialize with,
 *				it can contain null characters
 * @param[in] size length of the given data
 */
inline string_view::string_view(const char *data, size_t size)
    : _data(data), _size(size)
{
}

/**
 * Constructor initialized by the string *s*.
 *
 * @param[in] s reference to the string to initialize with
 */
inline string_view::string_view(const std::string &s)
    : _data(s.c_str()), _size(s.size())
{
}

/**
 * Constructor initialized by *data*. Size of the data will be set
 * using std::char_traits<char>::length().
 *
 * @param[in] data pointer to C-like string (char *) to initialize with,
 *				it has to end with the terminating null
 *character
 */
inline string_view::string_view(const char *data)
    : _data(data), _size(std::char_traits<char>::length(data))
{
}

/**
 * Returns pointer to data stored in this pmem::obj::string_view. It may not
 * contain the terminating null character.
 *
 * @return pointer to C-like string (char *), it may not end with null character
 */
inline const char *
string_view::data() const noexcept
{
	return _data;
}

/**
 * Returns count of characters stored in this pmem::obj::string_view data.
 *
 * @return pointer to C-like string (char *), it may not end with null character
 */
inline std::size_t
string_view::size() const noexcept
{
	return _size;
}

/**
 * Returns reference to a character at position @param[in] p
 *
 * @return reference to a char
 */
inline const char &string_view::operator[](size_t p) const noexcept
{
	return data()[p];
}

/**
 * Compares this string_view with other. Works in the same way as
 * std::basic_string::compare.
 *
 * @return 0 if both character sequences compare equal,
 *			positive value if this is lexicographically greater than
 *other, negative value if this is lexicographically less than other.
 */
inline int
string_view::compare(const string_view &other) noexcept
{
	int ret = std::char_traits<char>::compare(
		data(), other.data(), (std::min)(size(), other.size()));
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
