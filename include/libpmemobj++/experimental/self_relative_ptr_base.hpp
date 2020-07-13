// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * Base class for self_relative_ptr.
 */

#ifndef LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_HPP
#define LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_HPP

#include <cstdint>
#include <type_traits>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/specialization.hpp>

namespace pmem
{

namespace obj
{

namespace experimental
{
/**
 * self_relative_ptr base (non-template) class
 *
 * Implements some of the functionality of the self_relative_ptr class. It
 * defines all applicable conversions from and to a self_relative_ptr_base.
 *
 * It can be used e.g. as a parameter, where self_relative_ptr of any template
 * type is required. It is similar to self_relative_ptr<void> (it can point
 * to whatever type), but it can be used when you want to have pointer to some
 * unspecified self_relative_ptr (with self_relative_ptr<void> it can't be done,
 * because: self_relative_ptr<T>* does not convert to self_relative_ptr<void>*).
 */
class self_relative_ptr_base {
public:
	using this_type = self_relative_ptr_base;
	using difference_type = std::ptrdiff_t;
	using byte_type = uint8_t;
	using byte_ptr_type = byte_type *;
	using const_byte_ptr_type = const byte_type *;

	self_relative_ptr_base() noexcept : offset(pointer_to_offset(nullptr))
	{
	}

	self_relative_ptr_base(void *ptr) noexcept
	    : offset(pointer_to_offset(static_cast<byte_ptr_type>(ptr)))
	{
	}

	/*
	 * Copy constructor.
	 *
	 * @param r pointer to the same type.
	 */
	self_relative_ptr_base(self_relative_ptr_base const &r) noexcept
	    : offset(r.offset + distance_between_self(r))
	{
	}

	/**
	 * Assignment operator.
	 *
	 * Self-relative pointer assignment within a transaction
	 * automatically registers this operation so that a rollback
	 * is possible.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	self_relative_ptr_base &
	operator=(self_relative_ptr_base const &r)
	{
		if (this == &r)
			return *this;
		detail::conditional_add_to_tx(this);
		offset = r.offset + distance_between_self(r);
		return *this;
	}

	/**
	 * Nullptr move assignment operator.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	self_relative_ptr_base &
	operator=(std::nullptr_t &&)
	{
		detail::conditional_add_to_tx(this);
		offset = pointer_to_offset(nullptr);
		return *this;
	}

	/**
	 * Swaps two self_relative_ptr_base objects of the same type.
	 *
	 * @param[in,out] other the other self_relative_ptr to swap.
	 */
	void
	swap(self_relative_ptr_base &other)
	{
		if (this == &other)
			return;
		detail::conditional_add_to_tx(this);
		detail::conditional_add_to_tx(&other);
		auto first = this->to_byte_pointer();
		auto second = other.to_byte_pointer();
		this->offset = pointer_to_offset(second);
		other.offset = other.pointer_to_offset(first);
	}

	/**
	 * Conversion to persistent ptr
	 */
	byte_ptr_type
	to_byte_pointer() const noexcept
	{
		if (is_null())
			return nullptr;
		return reinterpret_cast<byte_ptr_type>(
			       const_cast<this_type *>(this)) +
			offset + 1;
	}

	/**
	 * Conversion to void*
	 */
	void *
	to_void_pointer() const noexcept
	{
		return static_cast<void *>(to_byte_pointer());
	}

	/**
	 * Explicit conversion operator to void*
	 */
	explicit operator void *() const noexcept
	{
		return to_void_pointer();
	}

	/**
	 * Explicit conversion operator to byte_pointer
	 */
	explicit operator byte_ptr_type() const noexcept
	{
		return to_byte_pointer();
	}

	/**
	 * Byte distance between two relative pointers
	 */
	static difference_type
	distance_between(const self_relative_ptr_base &first,
			 const self_relative_ptr_base &second)
	{
		return second.to_byte_pointer() - first.to_byte_pointer();
	}

	/**
	 * Fast null checking without conversion to void*
	 */
	inline bool
	is_null() const noexcept
	{
		return offset == nullptr_offset;
	}

protected:
	self_relative_ptr_base(difference_type offset) noexcept : offset(offset)
	{
	}

	/**
	 * Self distance between two relative pointers
	 */
	difference_type
	distance_between_self(const self_relative_ptr_base &ptr)
	{
		return reinterpret_cast<const_byte_ptr_type>(&ptr) -
			reinterpret_cast<const_byte_ptr_type>(this);
	}

	/**
	 * Conversion byte pointer to offset
	 */
	difference_type
	pointer_to_offset(void *ptr) const noexcept
	{
		if (ptr == nullptr)
			return nullptr_offset;
		return static_cast<byte_ptr_type>(ptr) -
			reinterpret_cast<const_byte_ptr_type>(this) - 1;
	}

	/* The offset from self */
	difference_type offset;

private:
	static constexpr difference_type nullptr_offset = 0;
};

} /* namespace obj */

} /* namespace pmem */

} /* namespace experimental */

#endif /* LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_HPP */
