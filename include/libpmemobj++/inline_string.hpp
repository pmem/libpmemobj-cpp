// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * Inline string implementation.
 */

#ifndef LIBPMEMOBJ_CPP_INLINE_STRING_HPP
#define LIBPMEMOBJ_CPP_INLINE_STRING_HPP

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#include <libpmemobj++/string_view.hpp>

#include <string>

namespace pmem
{

namespace obj
{

namespace experimental
{

template <typename T>
struct real_size {
	template <typename... Args>
	static size_t
	value(const Args &... args)
	{
		return sizeof(T);
	}
};

class inline_string {
public:
	using value_type = char;

	inline_string(string_view v) : size_(v.size())
	{
		std::copy(v.data(), v.data() + static_cast<ptrdiff_t>(size_),
			  data());
	}

	inline_string(const inline_string &rhs) : size_(rhs.size())
	{
		std::copy(rhs.data(),
			  rhs.data() + static_cast<ptrdiff_t>(size_), data());
	}

	operator string_view() const
	{
		return {data(), size()};
	}

	size_t
	size() const noexcept
	{
		return size_;
	}

	char *
	data() noexcept
	{
		return reinterpret_cast<char *>(this + 1);
	}

	const char *
	data() const noexcept
	{
		return reinterpret_cast<const char *>(this + 1);
	}

	inline_string &
	assign(string_view rhs)
	{
		auto pop = obj::pool_base(pmemobj_pool_by_ptr(this));

		auto initialized_mem = (std::min)(rhs.size(), size());

		obj::transaction::run(pop, [&] {
			detail::conditional_add_to_tx(data(), initialized_mem);

			if (rhs.size() > initialized_mem)
				detail::conditional_add_to_tx(
					data() + initialized_mem,
					rhs.size() - initialized_mem,
					POBJ_XADD_NO_SNAPSHOT);

			std::copy(rhs.data(),
				  rhs.data() +
					  static_cast<ptrdiff_t>(rhs.size()),
				  data());
			size_ = rhs.size();
		});

		return *this;
	}

private:
	obj::p<uint64_t> size_;
};

template <>
struct real_size<inline_string> {
	static size_t
	value(const string_view &s)
	{
		return sizeof(inline_string) + s.size();
	}
};

}
}
}

#endif /* LIBPMEMOBJ_CPP_INLINE_STRING_HPP */
