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

class inline_string {
	template <bool>
	class accessor;

public:
	using value_type = char;
	using reference = accessor<false>;
	using const_reference = accessor<true>;

	inline_string(string_view v, char *dst) : size_(v.size())
	{
		std::copy(v.data(), v.data() + static_cast<ptrdiff_t>(size_),
			  dst);
	}

	size_t
	size()
	{
		return size_;
	}

private:
	obj::p<uint64_t> size_;

	template <bool Const = false>
	class accessor {
	public:
		using pointer = typename std::conditional<Const, const char *,
							  char *>::type;
		using reference =
			typename std::conditional<Const, const inline_string &,
						  inline_string &>::type;

		accessor(pointer mem_begin, reference v)
		    : mem_begin(mem_begin), v(v)
		{
		}

		operator string_view() const
		{
			return {mem_begin, v.size_};
		}

		accessor &
		assign(string_view rhs)
		{
			auto pop = obj::pool_base(pmemobj_pool_by_ptr(&v));

			auto initialized_mem = (std::min)(rhs.size(), v.size());

			obj::transaction::run(pop, [&] {
				detail::conditional_add_to_tx(mem_begin,
							      initialized_mem);

				if (rhs.size() > initialized_mem)
					detail::conditional_add_to_tx(
						mem_begin + initialized_mem,
						rhs.size() - initialized_mem,
						POBJ_XADD_NO_SNAPSHOT);

				std::copy(rhs.data(),
					  rhs.data() +
						  +static_cast<ptrdiff_t>(
							  rhs.size()),
					  mem_begin);
				v.size_ = rhs.size();
			});

			return *this;
		}

	private:
		pointer mem_begin;
		reference v;
	};
};

}
}
}

#endif /* LIBPMEMOBJ_CPP_INLINE_STRING_HPP */
