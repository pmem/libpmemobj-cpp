// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * Persistent self-relative smart pointer.
 */

#ifndef LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_HPP
#define LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_HPP

#include <libpmemobj++/detail/specialization.hpp>
#include <libpmemobj++/experimental/self_relative_ptr_base.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <type_traits>

namespace pmem
{
namespace obj
{
namespace experimental
{

/**
 * Persistent self-relative pointer class.
 *
 * self_relative_ptr implements a smart ptr. It encapsulates the
 * self offseted pointer and provides member access, dereference and array
 * access operators.
 *
 * @includedoc shared/pointer_requirements.txt
 *
 * Casting to self_relative_ptr_base can be easily done from any
 * self_relative_ptr<T> objects, but when casting between convertible objects be
 * advised to use constructors or operator= specified for such conversion, see:
 * * self_relative_ptr::self_relative_ptr(self_relative_ptr<U> const &r) ,
 * * self_relative_ptr<T> & operator=(self_relative_ptr<Y> const &r) .
 * When casting indirectly with (void *) or using static_cast, and then casting
 * to the second (convertible) type, the offset will be re-calculated.
 *
 * @includedoc shared/self_relative_pointer_implementation.txt
 */
template <typename T>
class self_relative_ptr : public self_relative_ptr_base {
public:
	using base_type = self_relative_ptr_base;
	using this_type = self_relative_ptr;
	using element_type = typename pmem::detail::sp_element<T>::type;

	/**
	 * Random access iterator requirements (members)
	 */

	/**
	 * The self_relative_ptr iterator category.
	 */
	using iterator_category = std::random_access_iterator_tag;

	/**
	 * The self_relative_ptr difference type.
	 */
	using difference_type = typename base_type::difference_type;

	/**
	 * The type of the value pointed to by the self_relative_ptr.
	 */
	using value_type = T;

	/**
	 * The reference type of the value pointed to by the self_relative_ptr.
	 */
	using reference = T &;

	/*
	 * Constructors
	 */

	/**
	 * Default constructor, equal the nullptr
	 */
	constexpr self_relative_ptr() noexcept = default;

	/**
	 * Nullptr constructor
	 */
	constexpr self_relative_ptr(std::nullptr_t) noexcept
	    : self_relative_ptr_base()
	{
	}

	/**
	 * Volatile pointer constructor.
	 *
	 * @param ptr volatile pointer, pointing to persistent memory.
	 */
	self_relative_ptr(element_type *ptr) noexcept
	    : self_relative_ptr_base(self_offset(ptr))
	{
	}

	/**
	 * Constructor from persistent_ptr<T>
	 */
	self_relative_ptr(persistent_ptr<T> ptr) noexcept
	    : self_relative_ptr_base(self_offset(ptr.get()))
	{
	}

	/**
	 * PMEMoid constructor.
	 *
	 * Provided for easy interoperability between C++ and C API's.
	 *
	 * @param oid C-style persistent pointer
	 */
	self_relative_ptr(PMEMoid oid) noexcept
	    : self_relative_ptr_base(self_offset(
		      static_cast<element_type *>(pmemobj_direct(oid))))
	{
	}

	/**
	 * Copy constructor
	 */
	self_relative_ptr(const self_relative_ptr &ptr) noexcept
	    : self_relative_ptr_base(ptr)
	{
	}

	/**
	 * Copy constructor from a different self_relative_ptr<>.
	 *
	 * Available only for convertible, non-void types.
	 */
	template <
		typename U,
		typename = typename std::enable_if<
			!std::is_same<
				typename std::remove_cv<T>::type,
				typename std::remove_cv<U>::type>::value &&
				!std::is_void<U>::value,
			decltype(static_cast<T *>(std::declval<U *>()))>::type>
	self_relative_ptr(self_relative_ptr<U> const &r) noexcept
	    : self_relative_ptr_base(self_offset(static_cast<T *>(r.get())))
	{
	}

	~self_relative_ptr()
	{
		verify_type();
	}

	/**
	 * Get the direct pointer.
	 *
	 * @return the direct pointer to the object.
	 */
	inline element_type *
	get() const noexcept
	{
		return static_cast<element_type *>(
			self_relative_ptr_base::to_void_pointer());
	}

	/**
	 * Conversion to persitent ptr
	 */
	persistent_ptr<T>
	to_persistent_ptr() const
	{
		return persistent_ptr<T>{this->get()};
	}

	/*
	 * Operators
	 */

	/**
	 * Bool conversion operator.
	 */
	explicit operator bool() const noexcept
	{
		return !is_null();
	}

	/**
	 * Conversion operator to persistent_ptr
	 */
	operator persistent_ptr<T>() const
	{
		return to_persistent_ptr();
	}

	/**
	 * Dereference operator.
	 */
	typename pmem::detail::sp_dereference<T>::type operator*() const
		noexcept
	{
		return *(this->get());
	}

	/**
	 * Member access operator.
	 */
	typename pmem::detail::sp_member_access<T>::type operator->() const
		noexcept
	{
		return this->get();
	}

	/**
	 * Array access operator.
	 *
	 * Contains run-time bounds checking for static arrays.
	 */
	template <typename = typename std::enable_if<!std::is_void<T>::value>>
	typename pmem::detail::sp_array_access<T>::type
	operator[](difference_type i) const noexcept
	{
		assert(i >= 0 &&
		       (i < pmem::detail::sp_extent<T>::value ||
			pmem::detail::sp_extent<T>::value == 0) &&
		       "persistent array index out of bounds");

		return this->get()[i];
	}

	/**
	 * Assignment operator.
	 *
	 * self-relative pointer assignment within a transaction
	 * automatically registers this operation so that a rollback
	 * is possible.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	self_relative_ptr &
	operator=(const self_relative_ptr &r)
	{
		this->base_type::operator=(r);
		return *this;
	}

	/**
	 * Converting assignment operator from a different
	 * self_relative_ptr<>.
	 *
	 * Available only for convertible types.
	 * Just like regular assignment, also automatically registers
	 * itself in a transaction.
	 *
	 * @throw pmem::transaction_error when adding the object
	 * to the transaction failed.
	 */
	template <typename Y,
		  typename = typename std::enable_if<
			  std::is_convertible<Y *, T *>::value>::type>
	self_relative_ptr<T> &
	operator=(self_relative_ptr<Y> const &r)
	{
		this_type(r).swap(*this);
		return *this;
	}

	/**
	 * Nullptr move assignment operator.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	self_relative_ptr &operator=(std::nullptr_t)
	{
		detail::conditional_add_to_tx(this);
		this->offset = self_offset(nullptr);
		return *this;
	}

	/**
	 * Prefix increment operator.
	 */
	inline self_relative_ptr<T> &
	operator++()
	{
		detail::conditional_add_to_tx(this);
		this->offset += static_cast<difference_type>(sizeof(T));

		return *this;
	}

	/**
	 * Postfix increment operator.
	 */
	inline self_relative_ptr<T>
	operator++(int)
	{
		auto copy = *this;
		++(*this);

		return copy;
	}

	/**
	 * Prefix decrement operator.
	 */
	inline self_relative_ptr<T> &
	operator--()
	{
		detail::conditional_add_to_tx(this);
		this->offset -= static_cast<difference_type>(sizeof(T));

		return *this;
	}

	/**
	 * Postfix decrement operator.
	 */
	inline self_relative_ptr<T>
	operator--(int)
	{
		auto copy = *this;
		--(*this);

		return copy;
	}

	/**
	 * Addition assignment operator.
	 */
	inline self_relative_ptr<T> &
	operator+=(std::ptrdiff_t s)
	{
		detail::conditional_add_to_tx(this);
		this->offset += s * static_cast<difference_type>(sizeof(T));
		return *this;
	}

	/**
	 * Subtraction assignment operator.
	 */
	inline self_relative_ptr<T> &
	operator-=(std::ptrdiff_t s)
	{
		detail::conditional_add_to_tx(this);
		this->offset -= s * static_cast<difference_type>(sizeof(T));
		return *this;
	}

protected:
	/**
	 * Verify if element_type is not polymorphic
	 */
	void
	verify_type()
	{
		static_assert(!std::is_polymorphic<element_type>::value,
			      "Polymorphic types are not supported");
	}

private:
	difference_type
	self_offset(element_type *ptr) const noexcept
	{
		return base_type::pointer_to_offset(static_cast<void *>(ptr));
	}
};

/**
 * Swaps two self_relative_ptr objects of the same type.
 *
 * Non-member swap function as required by Swappable concept.
 * en.cppreference.com/w/cpp/concept/Swappable
 */
template <class T>
inline void
swap(self_relative_ptr<T> &a, self_relative_ptr<T> &b)
{
	a.swap(b);
}

/**
 * Equality operator.
 */
template <typename T, typename Y>
inline bool
operator==(self_relative_ptr<T> const &lhs,
	   self_relative_ptr<Y> const &rhs) noexcept
{
	return lhs.to_byte_pointer() == rhs.to_byte_pointer();
}

/**
 * Inequality operator.
 */
template <typename T, typename Y>
inline bool
operator!=(self_relative_ptr<T> const &lhs,
	   self_relative_ptr<Y> const &rhs) noexcept
{
	return !(lhs == rhs);
}

/**
 * Equality operator with nullptr.
 */
template <typename T>
inline bool
operator==(self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return !bool(lhs);
}

/**
 * Equality operator with nullptr.
 */
template <typename T>
inline bool
operator==(std::nullptr_t, self_relative_ptr<T> const &lhs) noexcept
{
	return !bool(lhs);
}

/**
 * Inequality operator with nullptr.
 */
template <typename T>
inline bool
operator!=(self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return bool(lhs);
}

/**
 * Inequality operator with nullptr.
 */
template <typename T>
inline bool
operator!=(std::nullptr_t, self_relative_ptr<T> const &lhs) noexcept
{
	return bool(lhs);
}

/**
 * Less than operator.
 *
 * @return true if the sum(this, offset) of lhs is less than the sum(this,
 * offset) of rhs. Returns false otherwise.
 */
template <typename T, typename Y>
inline bool
operator<(self_relative_ptr<T> const &lhs,
	  self_relative_ptr<Y> const &rhs) noexcept
{
	return lhs.to_byte_pointer() < rhs.to_byte_pointer();
}

/**
 * Less or equal than operator.
 *
 * See less than operator for comparison rules.
 */
template <typename T, typename Y>
inline bool
operator<=(self_relative_ptr<T> const &lhs,
	   self_relative_ptr<Y> const &rhs) noexcept
{
	return !(rhs < lhs);
}

/**
 * Greater than operator.
 *
 * See less than operator for comparison rules.
 */
template <typename T, typename Y>
inline bool
operator>(self_relative_ptr<T> const &lhs,
	  self_relative_ptr<Y> const &rhs) noexcept
{
	return (rhs < lhs);
}

/**
 * Greater or equal than operator.
 *
 * See less than operator for comparison rules.
 */
template <typename T, typename Y>
inline bool
operator>=(self_relative_ptr<T> const &lhs,
	   self_relative_ptr<Y> const &rhs) noexcept
{
	return !(lhs < rhs);
}

/* nullptr comparisons */

/**
 * Compare a self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<(self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return std::less<typename self_relative_ptr<T>::element_type *>()(
		lhs.get(), nullptr);
}

/**
 * Compare a self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<(std::nullptr_t, self_relative_ptr<T> const &rhs) noexcept
{
	return std::less<typename self_relative_ptr<T>::element_type *>()(
		nullptr, rhs.get());
}

/**
 * Compare a self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<=(self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return !(nullptr < lhs);
}

/**
 * Compare a self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<=(std::nullptr_t, self_relative_ptr<T> const &rhs) noexcept
{
	return !(rhs < nullptr);
}

/**
 * Compare a self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>(self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return nullptr < lhs;
}

/**
 * Compare a self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>(std::nullptr_t, self_relative_ptr<T> const &rhs) noexcept
{
	return rhs < nullptr;
}

/**
 * Compare a self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>=(self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return !(lhs < nullptr);
}

/**
 * Compare a self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>=(std::nullptr_t, self_relative_ptr<T> const &rhs) noexcept
{
	return !(nullptr < rhs);
}

/**
 * Addition operator for self-relative pointers.
 */
template <typename T>
inline self_relative_ptr<T>
operator+(self_relative_ptr<T> const &lhs, std::ptrdiff_t s)
{
	self_relative_ptr<T> ptr = lhs;
	ptr += s;
	return ptr;
}

/**
 * Subtraction operator for self-relative pointers.
 */
template <typename T>
inline self_relative_ptr<T>
operator-(self_relative_ptr<T> const &lhs, std::ptrdiff_t s)
{
	self_relative_ptr<T> ptr = lhs;
	ptr -= s;
	return ptr;
}

/**
 * Subtraction operator for self-relative pointers of identical type.
 *
 * Calculates the offset difference.
 * Calculating the difference of pointers from objects of
 * different pools is not allowed.
 */
template <typename T, typename Y,
	  typename = typename std::enable_if<
		  std::is_same<typename std::remove_cv<T>::type,
			       typename std::remove_cv<Y>::type>::value>>
inline ptrdiff_t
operator-(self_relative_ptr<T> const &lhs, self_relative_ptr<Y> const &rhs)
{
	return self_relative_ptr_base::distance_between(rhs, lhs) /
		static_cast<ptrdiff_t>(sizeof(T));
}

/**
 * Ostream operator
 */
template <typename T>
std::ostream &
operator<<(std::ostream &os, self_relative_ptr<T> const &ptr)
{
	os << ptr.to_void_pointer();
	return os;
}

} /* namespace experimental */

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_SELF_RELATIVE_PTR_HPP */
