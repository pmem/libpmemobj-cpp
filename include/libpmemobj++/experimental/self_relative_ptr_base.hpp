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
	using diff_t = std::ptrdiff_t;
	using byte = uint8_t;
	using byte_pointer = byte *;
	using const_byte_pointer = const byte *;

	self_relative_ptr_base() noexcept : offset(pointer_to_offset(nullptr))
	{
	}

	self_relative_ptr_base(void *ptr) noexcept
	    : offset(pointer_to_offset(static_cast<byte_pointer>(ptr)))
	{
	}

	/*
	 * Copy constructor.
	 *
	 * @param r Persistent pointer to the same type.
	 */
	self_relative_ptr_base(self_relative_ptr_base const &r) noexcept
	    : offset(r.offset + distance_between_self(*this, r))
	{
	}

	/**
	 * Assignment operator.
	 *
	 * Persistent pointer assignment within a transaction
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
		offset = r.offset + distance_between_self(*this, r);
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

	byte_pointer
	to_byte_pointer() const noexcept
	{
#if 0
		if (offset == 1) return nullptr;
		return reinterpret_cast<byte_pointer>(
			       const_cast<this_type *>(this)) +
			offset;
#else
		static_assert(sizeof(uintptr_t) == sizeof(byte_pointer));
		uintptr_t mask = offset == 1;
		--mask;
		uintptr_t ptr = reinterpret_cast<uintptr_t>(
			reinterpret_cast<byte_pointer>(
				const_cast<this_type *>(this)) +
			offset);
		ptr &= mask;
		return reinterpret_cast<byte_pointer>(ptr);
#endif
	}

	void *
	to_void_pointer() const noexcept
	{
		return static_cast<void *>(to_byte_pointer());
	}

	static diff_t
	distance_between(const self_relative_ptr_base &first,
			 const self_relative_ptr_base &second)
	{
		return second.to_byte_pointer() - first.to_byte_pointer();
	}

protected:
	self_relative_ptr_base(diff_t offset) noexcept : offset(offset)
	{
	}

	/**
	 * Self distance between two relative pointers
	 */
	static diff_t
	distance_between_self(const self_relative_ptr_base &first,
			      const self_relative_ptr_base &second)
	{
		return reinterpret_cast<const_byte_pointer>(&second) -
			reinterpret_cast<const_byte_pointer>(&first);
	}

	diff_t
	pointer_to_offset(byte_pointer ptr) const noexcept
	{
		if (ptr == nullptr)
			return 1;
		return ptr -
			reinterpret_cast<byte_pointer>(
				const_cast<this_type *>(this));
	}

	/* The offset from self */
	diff_t offset;
};

} /* namespace obj */

} /* namespace pmem */

} /* namespace experimental */

#endif /* LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_HPP */
