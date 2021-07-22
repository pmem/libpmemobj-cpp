// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, 4Paradigm Inc. */

#ifndef LIBPMEMOBJ_CPP_PA_SELF_RELATIVE_PTR_HPP
#define LIBPMEMOBJ_CPP_PA_SELF_RELATIVE_PTR_HPP

#include <libpmemobj++/detail/specialization.hpp>
#include <libpmemobj++/experimental/self_relative_ptr_base.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <type_traits>

/* According to the definition of offset=real_offset-1, for 8-byte aligned
 * allocation, the lower 3 bits of the stored offset are always 1 (except
 * null_ptr). Therefore, the second lowest bit is used as the indicator of if
 * the data pointed by the pa_self_relative_ptr
 * (persistent-aware self_relative_ptr) needs explicit flush.
 * Flush is needed if it is 0, not needed if it is 1.
 * */

#define kFlushNeeded ~(1L << 1)
// flag &= kFlushNeeded, to indicate it needs flush
#define FlushNeeded(offset) (!((offset >> 1) & 1))
// return true if needs explicit flush, false otherwise.

namespace pmem
{
namespace obj
{
namespace experimental
{

template <typename T>
class pa_self_relative_ptr;

template <>
class pa_self_relative_ptr<void> : public self_relative_ptr_base {
public:
	using base_type = self_relative_ptr_base;
	using this_type = pa_self_relative_ptr;
	using element_type = void;

	constexpr pa_self_relative_ptr() noexcept = default;

	constexpr pa_self_relative_ptr(std::nullptr_t) noexcept
	    : self_relative_ptr_base()
	{
	}

	pa_self_relative_ptr(element_type *ptr) noexcept
	    : self_relative_ptr_base(self_offset(ptr))
	{
	}

	inline element_type *
	get() const noexcept
	{
		return static_cast<element_type *>(this->to_void_pointer());
	}

private:
	difference_type
	self_offset(element_type *ptr) const noexcept
	{
		return base_type::pointer_to_offset(static_cast<void *>(ptr));
	}
};

template <typename T>
class pa_self_relative_ptr : public self_relative_ptr_base {
public:
	using base_type = self_relative_ptr_base;
	using this_type = pa_self_relative_ptr;
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
	constexpr pa_self_relative_ptr() noexcept = default;

	/**
	 * Nullptr constructor
	 */
	constexpr pa_self_relative_ptr(std::nullptr_t) noexcept
	    : self_relative_ptr_base()
	{
	}
	/**
	 * Volatile pointer constructor.
	 *
	 * @param ptr volatile pointer, pointing to persistent memory.
	 */
	pa_self_relative_ptr(element_type *ptr,
			     bool flushNeeded = false) noexcept
	    : base_type(self_offset(ptr))
	{
		intptr_t mask = (flushNeeded == true);
		--mask;
		this->offset &= (mask | kFlushNeeded);
	}

	/**
	 * Constructor from persistent_ptr<T>
	 */
	pa_self_relative_ptr(persistent_ptr<T> ptr,
			     bool flushNeeded = false) noexcept
	    : base_type(self_offset(ptr.get()))
	{
		intptr_t mask = (flushNeeded == true);
		--mask;
		this->offset &= (mask | kFlushNeeded);
	}

	/**
	 * PMEMoid constructor.
	 *
	 * Provided for easy interoperability between C++ and C API's.
	 *
	 * @param oid C-style persistent pointer
	 */
	pa_self_relative_ptr(PMEMoid oid, bool flushNeeded = false) noexcept
	    : base_type(self_offset(
		      static_cast<element_type *>(pmemobj_direct(oid))))
	{
		intptr_t mask = (flushNeeded == true);
		--mask;
		this->offset &= (mask | kFlushNeeded);
	}

	/**
	 * Copy constructor
	 */
	pa_self_relative_ptr(const pa_self_relative_ptr &ptr) noexcept
	    : base_type(ptr)
	{
		this->offset &= ptr.flush_set_mask();
	}

	/**
	 * Copy constructor from a different pa_self_relative_ptr<>.
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
	pa_self_relative_ptr(pa_self_relative_ptr<U> const &r) noexcept
	    : base_type(self_offset(static_cast<T *>(r.get())))
	{
		this->offset &= r.flush_set_mask();
	}

	~pa_self_relative_ptr()
	{
		verify_type();
	}
	/**
	 * Swaps two self_relative_ptr_base objects of the same type.
	 *
	 * @param[in,out] other the other self_relative_ptr to swap.
	 */
	void
	swap(pa_self_relative_ptr &other)
	{
		if (this == &other)
			return;
		detail::conditional_add_to_tx(this);
		detail::conditional_add_to_tx(&other);
		auto first = this->to_byte_pointer();
		auto mask = this->flush_set_mask();
		auto second = other.to_byte_pointer();
		this->offset = pointer_to_offset(second);
		this->offset &= other.flush_set_mask();
		other.offset = other.pointer_to_offset(first);
		other.offset &= mask;
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
		return this->offset_to_pointer(this->offset);
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
	distance_between(const pa_self_relative_ptr &first,
			 const pa_self_relative_ptr &second)
	{
		return second.to_byte_pointer() - first.to_byte_pointer();
	}

	/**
	 * Get the direct pointer.
	 *
	 * @return the direct pointer to the object.
	 */
	inline element_type *
	get() const noexcept
	{
		return static_cast<element_type *>(this->to_void_pointer());
	}

	/**
	 * Conversion to persitent ptr
	 */
	persistent_ptr<T>
	to_persistent_ptr() const
	{
		return persistent_ptr<T>{this->get()};
	}

	/**
	 * Check if flush is needed
	 */
	inline bool
	flush_needed() const
	{
		return (!is_null() && FlushNeeded(this->offset));
	}

	static inline bool
	flush_needed(offset_type offset)
	{
		return ((offset != nullptr_offset) && FlushNeeded(offset));
	}

	/**
	 * return mask for caller to & in order to set the flush_needed flag.
	 * can also be used to clear the flag using offset |= ~flush_set_mask().
	 */
	inline intptr_t
	flush_set_mask() const
	{
		intptr_t mask = flush_needed();
		--mask;
		return (mask | kFlushNeeded);
	}
	/**
	 * static version of flush_set_mask() for the given offset.
	 */
	static inline intptr_t
	flush_set_mask(offset_type offset)
	{
		intptr_t mask = this_type::flush_needed(offset);
		--mask;
		return (mask | kFlushNeeded);
	}

	/**
	 * return offset for debug only
	 */
	offset_type
	get_offset() const
	{
		return this->offset;
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
	pa_self_relative_ptr &
	operator=(const pa_self_relative_ptr &r)
	{
		this->base_type::operator=(r);
		return *this;
	}

	/**
	 * Converting assignment operator from a different
	 * pa_self_relative_ptr<>.
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
	pa_self_relative_ptr<T> &
	operator=(pa_self_relative_ptr<Y> const &r)
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
	pa_self_relative_ptr &operator=(std::nullptr_t)
	{
		detail::conditional_add_to_tx(this);
		this->offset = self_offset(nullptr);
		return *this;
	}

	/**
	 * Prefix increment operator.
	 */
	inline pa_self_relative_ptr<T> &
	operator++()
	{
		detail::conditional_add_to_tx(this);
		intptr_t mask = (this->flush_needed() == true);
		--mask;
		this->offset = (mask | kFlushNeeded) &
			((this->offset | ~kFlushNeeded) +
			 static_cast<difference_type>(sizeof(T)));
		return *this;
	}

	/**
	 * Postfix increment operator.
	 */
	inline pa_self_relative_ptr<T>
	operator++(int)
	{
		auto copy = *this;
		++(*this);

		return copy;
	}

	/**
	 * Prefix decrement operator.
	 */
	inline pa_self_relative_ptr<T> &
	operator--()
	{
		detail::conditional_add_to_tx(this);
		intptr_t mask = (this->flush_needed() == true);
		--mask;
		this->offset = (mask | kFlushNeeded) &
			((this->offset | ~kFlushNeeded) -
			 static_cast<difference_type>(sizeof(T)));
		return *this;
	}

	/**
	 * Postfix decrement operator.
	 */
	inline pa_self_relative_ptr<T>
	operator--(int)
	{
		auto copy = *this;
		--(*this);

		return copy;
	}

	/**
	 * Addition assignment operator.
	 */
	inline pa_self_relative_ptr<T> &
	operator+=(std::ptrdiff_t s)
	{
		detail::conditional_add_to_tx(this);
		intptr_t mask = (this->flush_needed() == true);
		--mask;
		this->offset = (mask | kFlushNeeded) &
			((this->offset | ~kFlushNeeded) +
			 s * static_cast<difference_type>(sizeof(T)));
		return *this;
	}

	/**
	 * Subtraction assignment operator.
	 */
	inline pa_self_relative_ptr<T> &
	operator-=(std::ptrdiff_t s)
	{
		detail::conditional_add_to_tx(this);
		intptr_t mask = (this->flush_needed() == true);
		--mask;
		this->offset = (mask | kFlushNeeded) &
			((this->offset | ~kFlushNeeded) -
			 s * static_cast<difference_type>(sizeof(T)));
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
	/**
	 * Conversion to void* use other offset
	 */
	void *
	offset_to_pointer(difference_type other_offset) const noexcept
	{
		intptr_t mask = other_offset == nullptr_offset;
		--mask;
		return base_type::offset_to_pointer(
			(other_offset | ~kFlushNeeded) & mask);
	}

private:
	static constexpr difference_type nullptr_offset = 0;
	difference_type
	self_offset(element_type *ptr) const noexcept
	{
		return base_type::pointer_to_offset(static_cast<void *>(ptr));
	}
	template <typename P>
	friend class pa_self_relative_accessor;
};

/**
 * Swaps two pa_self_relative_ptr objects of the same type.
 *
 * Non-member swap function as required by Swappable concept.
 * en.cppreference.com/w/cpp/concept/Swappable
 */
template <class T>
inline void
swap(pa_self_relative_ptr<T> &a, pa_self_relative_ptr<T> &b)
{
	a.swap(b);
}

/**
 * Equality operator.
 */
template <typename T, typename Y>
inline bool
operator==(pa_self_relative_ptr<T> const &lhs,
	   pa_self_relative_ptr<Y> const &rhs) noexcept
{
	return lhs.to_byte_pointer() == rhs.to_byte_pointer();
}

/**
 * Inequality operator.
 */
template <typename T, typename Y>
inline bool
operator!=(pa_self_relative_ptr<T> const &lhs,
	   pa_self_relative_ptr<Y> const &rhs) noexcept
{
	return !(lhs == rhs);
}

/**
 * Equality operator with nullptr.
 */
template <typename T>
inline bool
operator==(pa_self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return !bool(lhs);
}

/**
 * Equality operator with nullptr.
 */
template <typename T>
inline bool
operator==(std::nullptr_t, pa_self_relative_ptr<T> const &lhs) noexcept
{
	return !bool(lhs);
}

/**
 * Inequality operator with nullptr.
 */
template <typename T>
inline bool
operator!=(pa_self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return bool(lhs);
}

/**
 * Inequality operator with nullptr.
 */
template <typename T>
inline bool
operator!=(std::nullptr_t, pa_self_relative_ptr<T> const &lhs) noexcept
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
operator<(pa_self_relative_ptr<T> const &lhs,
	  pa_self_relative_ptr<Y> const &rhs) noexcept
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
operator<=(pa_self_relative_ptr<T> const &lhs,
	   pa_self_relative_ptr<Y> const &rhs) noexcept
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
operator>(pa_self_relative_ptr<T> const &lhs,
	  pa_self_relative_ptr<Y> const &rhs) noexcept
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
operator>=(pa_self_relative_ptr<T> const &lhs,
	   pa_self_relative_ptr<Y> const &rhs) noexcept
{
	return !(lhs < rhs);
}

/* nullptr comparisons */

/**
 * Compare a pa_self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<(pa_self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return std::less<typename pa_self_relative_ptr<T>::element_type *>()(
		lhs.get(), nullptr);
}

/**
 * Compare a pa_self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<(std::nullptr_t, pa_self_relative_ptr<T> const &rhs) noexcept
{
	return std::less<typename pa_self_relative_ptr<T>::element_type *>()(
		nullptr, rhs.get());
}

/**
 * Compare a pa_self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<=(pa_self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return !(nullptr < lhs);
}

/**
 * Compare a pa_self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<=(std::nullptr_t, pa_self_relative_ptr<T> const &rhs) noexcept
{
	return !(rhs < nullptr);
}

/**
 * Compare a pa_self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>(pa_self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return nullptr < lhs;
}

/**
 * Compare a pa_self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>(std::nullptr_t, pa_self_relative_ptr<T> const &rhs) noexcept
{
	return rhs < nullptr;
}

/**
 * Compare a pa_self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>=(pa_self_relative_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return !(lhs < nullptr);
}

/**
 * Compare a pa_self_relative_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>=(std::nullptr_t, pa_self_relative_ptr<T> const &rhs) noexcept
{
	return !(nullptr < rhs);
}

/**
 * Addition operator for self-relative pointers.
 */
template <typename T>
inline pa_self_relative_ptr<T>
operator+(pa_self_relative_ptr<T> const &lhs, std::ptrdiff_t s)
{
	pa_self_relative_ptr<T> ptr = lhs;
	ptr += s;
	return ptr;
}

/**
 * Subtraction operator for self-relative pointers.
 */
template <typename T>
inline pa_self_relative_ptr<T>
operator-(pa_self_relative_ptr<T> const &lhs, std::ptrdiff_t s)
{
	pa_self_relative_ptr<T> ptr = lhs;
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
operator-(pa_self_relative_ptr<T> const &lhs,
	  pa_self_relative_ptr<Y> const &rhs)
{
	return self_relative_ptr_base::distance_between(rhs, lhs) /
		static_cast<ptrdiff_t>(sizeof(T));
}

/**
 * Ostream operator
 */
template <typename T>
std::ostream &
operator<<(std::ostream &os, pa_self_relative_ptr<T> const &ptr)
{
	os << ptr.to_void_pointer();
	return os;
}

}
}
}

#endif // LIBPMEMOBJ_CPP_PA_SELF_RELATIVE_PTR_HPP
