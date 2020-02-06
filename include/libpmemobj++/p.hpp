// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2018, Intel Corporation */

/**
 * @file
 * Resides on pmem property template.
 */

#ifndef LIBPMEMOBJ_CPP_P_HPP
#define LIBPMEMOBJ_CPP_P_HPP

#include <memory>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/specialization.hpp>

namespace pmem
{

namespace obj
{
/**
 * Resides on pmem class.
 *
 * p class is a property-like template class that has to be used for all
 * variables (excluding persistent pointers), which are used in pmemobj
 * transactions. The p property makes sure that changes to a variable within
 * a transaction are made atomically with respect to persistence. It does it by
 * creating a snapshot of the variable when modified in the transaction scope.
 * The p class is not designed to be used with compound types. For that see the
 * persistent_ptr.
 * @snippet doc_snippets/persistent.cpp p_property_example
 */
template <typename T>
class p {
	typedef p<T> this_type;

public:
	/**
	 * Value constructor.
	 *
	 * Directly assigns a value to the underlying storage.
	 *
	 * @param _val const reference to the value to be assigned.
	 */
	p(const T &_val) noexcept : val{_val}
	{
	}

	/**
	 * Defaulted constructor.
	 */
	p() = default;

	/**
	 * Assignment operator.
	 *
	 * The p<> class property assignment within a transaction
	 * automatically registers this operation so that a rollback
	 * is possible.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	p &
	operator=(const p &rhs)
	{
		this_type(rhs).swap(*this);

		return *this;
	}

	/**
	 * Converting assignment operator from a different p<>.
	 *
	 * Available only for convertible types.
	 * Just like regular assignment, also automatically registers
	 * itself in a transaction.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	template <typename Y,
		  typename = typename std::enable_if<
			  std::is_convertible<Y, T>::value>::type>
	p &
	operator=(const p<Y> &rhs)
	{
		this_type(rhs).swap(*this);

		return *this;
	}

	/**
	 * Conversion operator back to the underlying type.
	 */
	operator T() const noexcept
	{
		return this->val;
	}

	/**
	 * Retrieves read-write reference of the object.
	 *
	 * The entire object is automatically added to the transaction.
	 *
	 * @return a reference to the object.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	T &
	get_rw()
	{
		detail::conditional_add_to_tx(this);

		return this->val;
	}

	/**
	 * Retrieves read-only const reference of the object.
	 *
	 * This method has no transaction side effects.
	 *
	 * @return a const reference to the object.
	 */
	const T &
	get_ro() const noexcept
	{
		return this->val;
	}

	/**
	 * Swaps two p objects of the same type.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	void
	swap(p &other)
	{
		detail::conditional_add_to_tx(this);
		detail::conditional_add_to_tx(&other);
		std::swap(this->val, other.val);
	}

private:
	T val;
};

/**
 * Swaps two p objects of the same type.
 *
 * Non-member swap function as required by Swappable concept.
 * en.cppreference.com/w/cpp/concept/Swappable
 */
template <class T>
inline void
swap(p<T> &a, p<T> &b)
{
	a.swap(b);
}

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_P_HPP */
