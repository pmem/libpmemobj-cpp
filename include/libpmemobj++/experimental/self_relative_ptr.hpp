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
 * Template parameter type has following requirements:
 * - Is not polymorphic
 * - Has no non-static data members of reference type
 * - Satisfies Destructible requirement:
 *   https://en.cppreference.com/w/cpp/named_req/Destructible
 * - All non-static data members and base classes follows the same requirements
 *
 * Even if all of the above requirements are met, type representation may vary
 * depending on ABI and compiler optimizations (as stated in [class.mem]: "the
 * order of allocation of non-static data members with different access control
 * is unspecified"). To enforce the same layout for all ABIs and optimization
 * levels type should satisfy StandardLayoutType requirement.
 *
 * If self_relative_ptr is used with array type, additional requirement is:
 * - Element type must be default constructible
 *
 * The self_relative_ptr is not designed to work with polymorphic
 * types, as they have runtime RTTI info embedded, which is implementation
 * specific and thus not consistently rebuildable. Such constructs as
 * polymorphic members or members of a union defined within a class held in
 * a self_relative_ptr will also yield undefined behavior.
 *
 * C++ standard states that lifetime of an object is a runtime property
 * [basic.lifetime]. Conditions which must be fulfilled for object's lifetime
 * to begin, imply that using any non-trivially constructible object with
 * self_relative_ptr is undefined behaviour. This is being partially addressed
 * by the following proposal:
 * https://groups.google.com/a/isocpp.org/forum/#!topic/std-proposals/bk8esqk-Qoo
 *
 * Another caveat is that snapshotting elements in a transaction and performing
 * rollback uses memcpy internally. Using memcpy on an object in C++ is allowed
 * by the standard only if the type satisfies TriviallyCopyable requirement.
 *
 *
 * Casting to self_relative_ptr_base can be easily done from any
 * self_relative_ptr<T> objects, but when casting between convertible objects be
 * advised to use constructors or operator= specified for such conversion, see:
 * * self_relative_ptr::self_relative_ptr(self_relative_ptr<U> const &r) ,
 * * self_relative_ptr<T> & operator=(self_relative_ptr<Y> const &r) .
 * When casting indirectly with (void *) or using static_cast, and then casting
 * to the second (convertible) type, the offset will not be re-calculated.
 */
template <typename T>
class self_relative_ptr : public self_relative_ptr_base {
public:
	using this_type = self_relative_ptr;
	using element_type = typename pmem::detail::sp_element<T>::type;
	using diff_t = std::ptrdiff_t;
	using byte_pointer = uint8_t *;

	static_assert(!std::is_polymorphic<element_type>::value,
		      "Polymorphic types are not supported");

	self_relative_ptr() noexcept = default;

	self_relative_ptr(std::nullptr_t) noexcept : self_relative_ptr_base()
	{
	}

	self_relative_ptr(element_type *ptr) noexcept
	    : self_relative_ptr_base(self_offset(ptr))
	{
	}

	self_relative_ptr(persistent_ptr<T> ptr) noexcept
	    : self_relative_ptr_base(self_offset(ptr.get()))
	{
	}

	self_relative_ptr(PMEMoid oid) noexcept
	    : self_relative_ptr_base(self_offset(
		      static_cast<element_type *>(pmemobj_direct(oid))))
	{
	}

	self_relative_ptr(const self_relative_ptr &ptr) noexcept
	    : self_relative_ptr_base(self_offset(ptr.get()))
	{
	}

	/**
	 * Copy constructor from a different persistent_ptr<>.
	 *
	 * Available only for convertible, non-void types.
	 */
	template <
		typename U, typename Dummy = void,
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

	inline element_type *
	get() const noexcept
	{
		return this_to_pointer();
	}

	persistent_ptr<T>
	to_persitent_ptr()
	{
		return persistent_ptr<T>{this->get()};
	}

	/*
	 * Operators
	 */

	/*
	 * Bool conversion operator.
	 */
	explicit operator bool() const noexcept
	{
		return get() != nullptr;
	}

	explicit operator persistent_ptr<T>() const noexcept
	{
		return persistent_ptr<T>{get()};
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
	operator[](std::ptrdiff_t i) const noexcept
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
	 * Persistent pointer assignment within a transaction
	 * automatically registers this operation so that a rollback
	 * is possible.
	 *
	 * @throw pmem::transaction_error when adding the object to the
	 *	transaction failed.
	 */
	self_relative_ptr &
	operator=(const self_relative_ptr &r)
	{
		detail::conditional_add_to_tx(this);
		this->offset = self_offset(r.get());

		return *this;
	}

	/**
	 * Converting assignment operator from a different
	 * persistent_ptr<>.
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
		this->offset += static_cast<diff_t>(sizeof(T));

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
		this->offset -= static_cast<diff_t>(sizeof(T));

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
		this->offset += s * static_cast<diff_t>(sizeof(T));
		return *this;
	}

	/**
	 * Subtraction assignment operator.
	 */
	inline self_relative_ptr<T> &
	operator-=(std::ptrdiff_t s)
	{
		detail::conditional_add_to_tx(this);
		this->offset -= s * static_cast<diff_t>(sizeof(T));
		return *this;
	}

	/**
	 * Random access iterator requirements (members)
	 */

	/**
	 * The persistent_ptr iterator category.
	 */
	using iterator_category = std::random_access_iterator_tag;

	/**
	 * The persistent_ptr difference type.
	 */
	using difference_type = std::ptrdiff_t;

	/**
	 * The type of the value pointed to by the persistent_ptr.
	 */
	using value_type = T;

	/**
	 * The reference type of the value pointed to by the persistent_ptr.
	 */
	using reference = T &;

private:
	diff_t
	self_offset(element_type *ptr) const noexcept
	{
		return self_relative_ptr_base::pointer_to_offset(
			reinterpret_cast<byte_pointer>(ptr));
	}

	element_type *
	this_to_pointer() const noexcept
	{
		return static_cast<element_type *>(
			self_relative_ptr_base::to_void_pointer());
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
 *
 * This checks if underlying PMEMoids are equal.
 */
template <typename T, typename Y>
inline bool
operator==(self_relative_ptr<T> const &lhs,
	   self_relative_ptr<Y> const &rhs) noexcept
{
	return lhs.get() == static_cast<T *>(rhs.get());
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
	return lhs.get() == nullptr;
}

/**
 * Equality operator with nullptr.
 */
template <typename T>
inline bool
operator==(std::nullptr_t, self_relative_ptr<T> const &lhs) noexcept
{
	return lhs.get() == nullptr;
}

/**
 * Inequality operator with nullptr.
 */
template <typename T>
inline bool
operator!=(self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return lhs.get() != nullptr;
}

/**
 * Inequality operator with nullptr.
 */
template <typename T>
inline bool
operator!=(std::nullptr_t, self_relative_ptr<T> const &lhs) noexcept
{
	return lhs.get() != nullptr;
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
	return *lhs < *rhs;
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
 * Addition operator for persistent pointers.
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
 * Subtraction operator for persistent pointers.
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
 * Subtraction operator for persistent pointers of identical type.
 *
 * Calculates the offset difference of PMEMoids in terms of represented
 * objects. Calculating the difference of pointers from objects of
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
