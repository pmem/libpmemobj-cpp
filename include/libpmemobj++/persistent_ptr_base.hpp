// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/**
 * @file
 * Base class for persistent_ptr.
 */

#ifndef LIBPMEMOBJ_CPP_PERSISTENT_PTR_BASE_HPP
#define LIBPMEMOBJ_CPP_PERSISTENT_PTR_BASE_HPP

#include <cstdint>
#include <type_traits>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/specialization.hpp>
#include <libpmemobj/base.h>

/* Windows has a max macro which collides with std::numeric_limits::max */
#if defined(max) && defined(_WIN32)
#undef max
#endif

namespace pmem
{

namespace obj
{

/**
 * Persistent_ptr base (non-template) class
 *
 * Implements some of the functionality of the persistent_ptr class. It defines
 * all applicable conversions from and to a persistent_ptr_base.
 *
 * It can be used e.g. as a parameter, where persistent_ptr of any template
 * type is required. It is similar to persistent_ptr<void> (it can point
 * to whatever type), but it can be used when you want to have pointer to some
 * unspecified persistent_ptr (with persistent_ptr<void> it can't be done,
 * because: persistent_ptr<T>* does not convert to persistent_ptr<void>*).
 */
class persistent_ptr_base {
public:
	/**
	 * Default constructor, zeroes the PMEMoid.
	 */
	persistent_ptr_base() noexcept : oid(OID_NULL)
	{
	}

	/*
	 * Curly braces initialization is not used because the
	 * PMEMoid is a plain C (POD) type and we can't add a default
	 * constructor in there.
	 */

	/**
	 * PMEMoid constructor.
	 *
	 * Provided for easy interoperability between C++ and C API's.
	 *
	 * @param oid C-style persistent pointer
	 */
	persistent_ptr_base(PMEMoid oid) noexcept : oid(oid)
	{
	}

	/*
	 * Copy constructor.
	 *
	 * @param r Persistent pointer to the same type.
	 */
	persistent_ptr_base(persistent_ptr_base const &r) noexcept : oid(r.oid)
	{
	}

	/**
	 * Move constructor.
	 */
	persistent_ptr_base(persistent_ptr_base &&r) noexcept
	    : oid(std::move(r.oid))
	{
	}

	/**
	 * Move assignment operator.
	 */
	persistent_ptr_base &
	operator=(persistent_ptr_base &&r)
	{
		detail::conditional_add_to_tx(this);
		this->oid = std::move(r.oid);

		return *this;
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
	persistent_ptr_base &
	operator=(persistent_ptr_base const &r)
	{
		detail::conditional_add_to_tx(this);
		this->oid = r.oid;

		return *this;
	}

	/**
	 * Nullptr move assignment operator.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	persistent_ptr_base &
	operator=(std::nullptr_t &&)
	{
		detail::conditional_add_to_tx(this);
		this->oid = {0, 0};
		return *this;
	}

	/**
	 * Swaps two persistent_ptr objects of the same type.
	 *
	 * @param[in,out] other the other persistent_ptr to swap.
	 */
	void
	swap(persistent_ptr_base &other)
	{
		detail::conditional_add_to_tx(this);
		detail::conditional_add_to_tx(&other);
		std::swap(this->oid, other.oid);
	}

	/**
	 * Get PMEMoid encapsulated by this object.
	 *
	 * For C API compatibility.
	 *
	 * @return const reference to the PMEMoid
	 */
	const PMEMoid &
	raw() const noexcept
	{
		return this->oid;
	}

	/**
	 * Get pointer to PMEMoid encapsulated by this object.
	 *
	 * For C API compatibility.
	 *
	 * @return pointer to the PMEMoid
	 */
	PMEMoid *
	raw_ptr() noexcept
	{
		return &(this->oid);
	}

protected:
	/* The underlying PMEMoid of the held object. */
	PMEMoid oid;
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_PERSISTENT_PTR_BASE_HPP */
