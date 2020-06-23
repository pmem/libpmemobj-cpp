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

class inline_string {
public:
	using value_type = char;
	using view_type = string_view;

	inline_string(view_type v, char *dst) : size_(v.size())
	{
		std::copy(v.data(), v.data() + size_, dst);
	}

	size_t
	size()
	{
		return size_;
	}

	class accessor {
	public:
		accessor(char *mem_begin, inline_string &v)
		    : mem_begin(mem_begin), v(v)
		{
		}

		view_type
		get() const
		{
			return {mem_begin, v.size_};
		}

		accessor &
		assign(view_type rhs)
		{
			auto pop = obj::pool_base(pmemobj_pool_by_ptr(&v));

			obj::transaction::run(pop, [&] {
				std::copy(rhs.data(), rhs.data() + rhs.size(),
					  mem_begin);
				v.size_ = rhs.size();
			});

			return *this;
		}

	private:
		char *mem_begin;
		inline_string &v;
	};

private:
	obj::p<uint64_t> size_;
};

}
}

#endif /* LIBPMEMOBJ_CPP_INLINE_STRING_HPP */
