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

/**
 * This class serves similar purpose to pmem::obj::string, but
 * keeps the data within the same allocation as inline_string itself.
 *
 * The data is always kept right after the inline_string structure.
 * It means that creating an object of inline_string must be done
 * as follows:
 * 1. Allocate memory of sizeof(inline_string) + required capacity
 * 2. Use emplace new() to create inline_string
 *
 * Example:
 * @snippet inline_string/inline_string.cpp inline_string_example
 */
class inline_string {
public:
	using value_type = char;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;

	/**
	 * Constructs inline string from a string_view.
	 *
	 * @throw pool_error if inline_string doesn't reside on pmem.
	 */
	inline_string(string_view v) : size_(v.size())
	{
		if (nullptr == pmemobj_pool_by_ptr(this))
			throw pmem::pool_error("Invalid pool handle.");

		std::copy(v.data(), v.data() + static_cast<ptrdiff_t>(size_),
			  data());
	}

	/**
	 * Copy constructor
	 *
	 * @throw pool_error if inline_string doesn't reside on pmem.
	 */
	inline_string(const inline_string &rhs) : size_(rhs.size())
	{
		if (nullptr == pmemobj_pool_by_ptr(this))
			throw pmem::pool_error("Invalid pool handle.");

		std::copy(rhs.data(),
			  rhs.data() + static_cast<ptrdiff_t>(size_), data());
	}

	/**
	 * Copy assignment operator
	 */
	inline_string &
	operator=(const inline_string &rhs)
	{
		return assign(rhs);
	}

	inline_string(inline_string &&) = delete;
	inline_string &operator=(inline_string &&) = delete;

	/* Conversion operator to string_view */
	operator string_view() const
	{
		return {data(), size()};
	}

	/** @return size of the string */
	size_type
	size() const noexcept
	{
		return size_;
	}

	/** @return pointer to the data (equal to (this + 1)) */
	pointer
	data() noexcept
	{
		return reinterpret_cast<char *>(this + 1);
	}

	/** @return const_pointer to the data (equal to (this + 1)) */
	const_pointer
	data() const noexcept
	{
		return reinterpret_cast<const char *>(this + 1);
	}

	/**
	 * Transactionally assign content of string_view.
	 *
	 * It does not check whether there is enough capacity - it's
	 * user responsibility.
	 */
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

/**
 * A helper trait which calculates required memory capacity for a type.
 *
 * All standard types require capacity equal to the sizeof of such type.
 */
template <typename T>
struct real_size {
	template <typename... Args>
	static size_t
	value(const Args &... args)
	{
		return sizeof(T);
	}
};

/**
 * A helper trait which calculates required memory capacity for a type.
 *
 * Inline_string requires capacity of sizeof(inline_string) + size of the
 * data itself.
 */
template <>
struct real_size<inline_string> {
	static size_t
	value(const string_view &s)
	{
		return sizeof(inline_string) + s.size();
	}
};

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_INLINE_STRING_HPP */
