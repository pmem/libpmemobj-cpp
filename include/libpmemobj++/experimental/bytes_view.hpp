// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_BYTES_VIEW_HPP
#define LIBPMEMOBJ_CPP_BYTES_VIEW_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace pmem
{

namespace obj
{

namespace experimental
{

template <typename T, typename Enable = void>
struct big_endian_bytes_view;

template <typename T>
struct big_endian_bytes_view<
	T,
	typename std::enable_if<std::is_integral<T>::value &&
				!std::is_signed<T>::value>::type> {
	big_endian_bytes_view(const T *k) : k(k)
	{
#ifndef NDEBUG
		/* Assert big endian is used. */
		uint16_t word = (2 << 8) + 1;
		assert(((char *)&word)[0] == 1);
#endif
	}

	char operator[](std::ptrdiff_t p) const
	{
		return reinterpret_cast<const char *>(
			k)[static_cast<ptrdiff_t>(size()) - p - 1];
	}

	constexpr size_t
	size() const
	{
		return sizeof(T);
	}

	const T *k;
};

}
}
}

#endif
