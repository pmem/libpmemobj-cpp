// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * Base class for self_relative_ptr.
 */

#ifndef LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_IMPL_HPP
#define LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_IMPL_HPP

#include <cstdint>
#include <type_traits>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/specialization.hpp>

namespace pmem
{

namespace detail
{

/**
 * self_relative_ptr base template class
 *
 * Implements some of the functionality of the self_relative_ptr class. It
 * defines all applicable conversions from and to a self_relative_ptr_base.
 *
 * It can be used e.g. as a parameter, where self_relative_ptr of any template
 * type is required. It is similar to self_relative_ptr<void> (it can point
 * to whatever type), but it can be used when you want to have pointer to some
 * unspecified self_relative_ptr (with self_relative_ptr<void> it can't be done,
 * because: self_relative_ptr<T>* does not convert to self_relative_ptr<void>*).
 *
 * @includedoc shared/self_relative_pointer_implementation.txt
 */
template <typename OffsetType>
class self_relative_ptr_base_impl {
public:
	using this_type = self_relative_ptr_base_impl;
	using difference_type = std::ptrdiff_t;
	using offset_type = OffsetType;
	using byte_type = uint8_t;
	using byte_ptr_type = byte_type *;
	using const_byte_ptr_type = const byte_type *;

	/*
	 * Constructors
	 */

	/**
	 * Default constructor, equal the nullptr
	 */
	constexpr self_relative_ptr_base_impl() noexcept
	    : offset(nullptr_offset)
	{
	}

	/**
	 * Nullptr constructor
	 */
	constexpr self_relative_ptr_base_impl(std::nullptr_t) noexcept
	    : offset(nullptr_offset)
	{
	}

	/**
	 * Volatile pointer constructor.
	 *
	 * @param ptr volatile pointer, pointing to persistent memory.
	 */
	self_relative_ptr_base_impl(void *ptr) noexcept
	    : offset(pointer_to_offset(ptr))
	{
	}

	/**
	 * Copy constructor.
	 *
	 * @param r pointer to the same type.
	 */
	self_relative_ptr_base_impl(
		self_relative_ptr_base_impl const &r) noexcept
	    : offset(pointer_to_offset(r))
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
	self_relative_ptr_base_impl &
	operator=(self_relative_ptr_base_impl const &r)
	{
		if (this == &r)
			return *this;
		detail::conditional_add_to_tx(this);
		offset = pointer_to_offset(r);
		return *this;
	}

	/**
	 * Nullptr move assignment operator.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	self_relative_ptr_base_impl &
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
	swap(self_relative_ptr_base_impl &other)
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
	 * Conversion to byte pointer
	 */
	byte_ptr_type
	to_byte_pointer() const noexcept
	{
		return static_cast<byte_ptr_type>(this->to_void_pointer());
	}

	/**
	 * Conversion to void*
	 */
	void *
	to_void_pointer() const noexcept
	{
		return offset_to_pointer(this->offset);
	}

	/**
	 * Explicit conversion operator to void*
	 */
	explicit operator void *() const noexcept
	{
		return to_void_pointer();
	}

	/**
	 * Explicit conversion operator to byte pointer
	 */
	explicit operator byte_ptr_type() const noexcept
	{
		return to_byte_pointer();
	}

	/**
	 * Byte distance between two relative pointers
	 */
	static difference_type
	distance_between(const self_relative_ptr_base_impl &first,
			 const self_relative_ptr_base_impl &second)
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
	/**
	 * Offset constructor.
	 *
	 * @param offset offset from self.
	 */
	self_relative_ptr_base_impl(difference_type offset) noexcept
	    : offset(offset)
	{
	}

	/**
	 * Conversion to void* use other offset
	 */
	void *
	offset_to_pointer(difference_type other_offset) const noexcept
	{
		/*
		This version without branches is vectorization-friendly.
		mask = is_null() should not create a branch in the code.
		In this line, we just assign 0 or 1 to the mask variable.

		This code is equal:
		if (is_null())
			return nullptr;
		return reinterpret_cast<byte_ptr_type>(
			       const_cast<this_type *>(this)) +
			offset + 1;
		*/
		uintptr_t mask = other_offset == nullptr_offset;
		--mask;
		uintptr_t ptr = reinterpret_cast<uintptr_t>(
			reinterpret_cast<const_byte_ptr_type>(this) +
			other_offset + 1);
		ptr &= mask;
		return reinterpret_cast<void *>(ptr);
	}

	/**
	 * Conversion self_relative_ptr_base to offset from itself
	 */
	difference_type
	pointer_to_offset(const self_relative_ptr_base_impl &ptr) const noexcept
	{
		/*
		This version without branches is vectorization-friendly.
		mask = is_null() should not create a branch in the code.
		In this line, we just assign 0 or 1 to the mask variable.

		This code is equal:
		return ptr.is_null()
			? nullptr_offset
			: ptr.offset + this->distance_between_self(ptr);
		*/
		uintptr_t mask = ptr.is_null();
		--mask;
		difference_type distance_between_self =
			reinterpret_cast<const_byte_ptr_type>(&ptr) -
			reinterpret_cast<const_byte_ptr_type>(this);
		distance_between_self &=
			reinterpret_cast<difference_type &>(mask);
		return ptr.offset + distance_between_self;
	}

	/**
	 * Conversion pointer to offset
	 */
	difference_type
	pointer_to_offset(void *ptr) const noexcept
	{
		/*
		This version without branches is vectorization-friendly.
		mask = ptr == nullptr should not create a branch in the code.
		In this line, we just assign 0 or 1 to the mask variable.

		This code is equal:
		if (ptr == nullptr)
		    return nullptr_offset
		else
		    return ptr - this - 1;
		*/
		uintptr_t mask = ptr == nullptr;
		--mask;
		difference_type new_offset = static_cast<byte_ptr_type>(ptr) -
			reinterpret_cast<const_byte_ptr_type>(this) - 1;
		new_offset &= reinterpret_cast<difference_type &>(mask);
		return new_offset;
	}

	/* The offset from self */
	offset_type offset;

private:
	static constexpr difference_type nullptr_offset = 0;

	template <typename T>
	friend class self_relative_accessor;
};

/**
 * Static class accesssor to self_relative_ptr_base
 */
template <typename T>
class self_relative_accessor {
public:
	using access_type = self_relative_ptr_base_impl<T>;
	using difference_type = typename access_type::difference_type;

	template <typename PointerType>
	static difference_type
	pointer_to_offset(const access_type &obj, PointerType *ptr)
	{
		return obj.pointer_to_offset(static_cast<void *>(ptr));
	}

	template <typename PointerType>
	static PointerType *
	offset_to_pointer(difference_type offset, const access_type &obj)
	{
		auto ptr = obj.offset_to_pointer(offset);
		return static_cast<PointerType *>(ptr);
	}

	static T &
	get_offset(access_type &ptr)
	{
		return ptr.offset;
	}

	static const T &
	get_offset(const access_type &ptr)
	{
		return ptr.offset;
	}
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_BASE_IMPL_HPP */
