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

#include <libpmemobj++/detail/pair.hpp>
#include <libpmemobj++/slice.hpp>
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
		accessor(obj::slice<char *> memory, inline_string &v)
		    : memory(memory), v(v)
		{
		}

		view_type
		get() const
		{
			return {memory.begin(), v.size_};
		}

		accessor &
		assign(view_type rhs)
		{
			if (rhs.size() > memory.size())
				throw std::out_of_range(
					"inline_vector::accessor::assign");

			auto pop = obj::pool_base(pmemobj_pool_by_ptr(&v));

			obj::transaction::run(pop, [&] {
				std::copy(rhs.data(), rhs.data() + rhs.size(),
					  memory.begin());
				v.size_ = rhs.size();
			});

			return *this;
		}

	private:
		const obj::slice<char *> memory;
		inline_string &v;
	};

private:
	obj::p<uint64_t> size_;
};

}
}

#endif /* LIBPMEMOBJ_CPP_INLINE_STRING_HPP */
