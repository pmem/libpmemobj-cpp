/*
 * Copyright 2015-2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * Persistent smart pointer.
 */

#ifndef LIBPMEMOBJ_CPP_PERSISTENT_PTR_HPP
#define LIBPMEMOBJ_CPP_PERSISTENT_PTR_HPP

#include <cassert>
#include <limits>
#include <memory>
#include <ostream>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/specialization.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/base.h>

namespace pmem
{

namespace obj
{

template <typename T>
class pool;

template <typename T>
class persistent_ptr;

/**
 * persistent_ptr void specialization.
 *
 * It's truncated specialization to disallow some of the
 * (unnecessary) functionalities.
 */
template <>
class persistent_ptr<void> : public persistent_ptr_base {
public:
	typedef void element_type;
	typedef persistent_ptr<void> this_type;

	persistent_ptr() = default;
	using persistent_ptr_base::persistent_ptr_base;
	persistent_ptr(void *ptr) : persistent_ptr_base(pmemobj_oid(ptr))
	{
	}

	element_type *
	get() const noexcept
	{
		if (this->oid.pool_uuid_lo ==
		    std::numeric_limits<decltype(oid.pool_uuid_lo)>::max())
			return reinterpret_cast<element_type *>(oid.off);
		else
			return static_cast<element_type *>(
				pmemobj_direct(this->oid));
	}

	template <typename Y,
		  typename = typename std::enable_if<
			  std::is_convertible<Y *, void *>::value>::type>
	persistent_ptr<void> &
	operator=(persistent_ptr<void> const &r)
	{
		this_type(r).swap(*this);

		return *this;
	}

	explicit operator bool() const noexcept
	{
		return get() != nullptr;
	}
};

/**
 * persistent_ptr const void specialization.
 *
 * It's truncated specialization to disallow some
 * of the (unnecessary) functionalities.
 */
template <>
class persistent_ptr<const void> : public persistent_ptr_base {
public:
	typedef const void element_type;
	typedef persistent_ptr<const void> this_type;

	persistent_ptr() = default;
	using persistent_ptr_base::persistent_ptr_base;
	persistent_ptr(const void *ptr) : persistent_ptr_base(pmemobj_oid(ptr))
	{
	}

	element_type *
	get() const noexcept
	{
		if (this->oid.pool_uuid_lo ==
		    std::numeric_limits<decltype(oid.pool_uuid_lo)>::max())
			return reinterpret_cast<element_type *>(oid.off);
		else
			return static_cast<element_type *>(
				pmemobj_direct(this->oid));
	}

	template <typename Y,
		  typename = typename std::enable_if<
			  std::is_convertible<Y *, const void *>::value>::type>
	persistent_ptr<const void> &
	operator=(persistent_ptr<const void> const &r)
	{
		this_type(r).swap(*this);

		return *this;
	}

	explicit operator bool() const noexcept
	{
		return get() != nullptr;
	}
};

/**
 * Persistent pointer class.
 *
 * persistent_ptr implements a smart ptr. It encapsulates the PMEMoid
 * fat pointer and provides member access, dereference and array
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
 * If persistent_ptr is used with array type, additional requirement is:
 * - Element type must be default constructible
 *
 * The persistent_ptr is not designed to work with polymorphic
 * types, as they have runtime RTTI info embedded, which is implementation
 * specific and thus not consistently rebuildable. Such constructs as
 * polymorphic members or members of a union defined within a class held in
 * a persistent_ptr will also yield undefined behavior.
 *
 * C++ standard states that lifetime of an object is a runtime property
 * [basic.lifetime]. Conditions which must be fulfilled for object's lifetime
 * to begin, imply that using any non-trivially constructible object with
 * persistent_ptr is undefined behaviour. This is being partially addressed by
 * the following proposal:
 * https://groups.google.com/a/isocpp.org/forum/#!topic/std-proposals/bk8esqk-Qoo
 *
 * Another caveat is that snapshotting elements in a transaction and performing
 * rollback uses memcpy internally. Using memcpy on an object in C++ is allowed
 * by the standard only if the type satisfies TriviallyCopyable requirement.
 *
 * This type does NOT manage the life-cycle of the object. The typical usage
 * example would be:
 * @snippet doc_snippets/persistent.cpp persistent_ptr_example
 *
 * Casting to persistent_ptr_base can be easily done from any persistent_ptr<T>
 * objects, but when casting between convertible objects be advised to use
 * constructors or operator= specified for such conversion, see:
 * * persistent_ptr::persistent_ptr(persistent_ptr<U> const &r) ,
 * * persistent_ptr<T> & operator=(persistent_ptr<Y> const &r) .
 * When casting indirectly with (void *) or using static_cast, and then casting
 * to the second (convertible) type, the offset will not be re-calculated.
 *
 * Below you can find an example how to and how NOT to cast persistent_ptr's:
 * @snippet doc_snippets/persistent.cpp persistent_ptr_casting_example
 */
template <typename T>
class persistent_ptr : public persistent_ptr_base {
public:
	typedef persistent_ptr<T> this_type;

	template <typename U>
	friend class persistent_ptr;

	/**
	 * Type of the actual object with all qualifiers removed,
	 * used for easy underlying type access.
	 */
	typedef typename pmem::detail::sp_element<T>::type element_type;

	persistent_ptr() = default;
	using persistent_ptr_base::persistent_ptr_base;

	/*
	 * Constructors
	 */

	/**
	 * Explicit void specialization of the converting constructor.
	 */
	explicit persistent_ptr(persistent_ptr<void> const &rhs) noexcept
	    : persistent_ptr_base(rhs.raw())
	{
	}

	/**
	 * Explicit const void specialization of the converting constructor.
	 */
	explicit persistent_ptr(persistent_ptr<const void> const &rhs) noexcept
	    : persistent_ptr_base(rhs.raw())
	{
	}

	/**
	 * Volatile pointer constructor.
	 *
	 * If ptr does not point to an address from a valid pool, the persistent
	 * pointer will evaluate to nullptr.
	 *
	 * @param ptr volatile pointer, pointing to persistent memory.
	 */
	persistent_ptr(element_type *ptr)
	    : persistent_ptr_base(pmemobj_oid(ptr))
	{
		verify_type();
	}

	/**
	 * Copy constructor from a different persistent_ptr<>.
	 *
	 * Available only for convertible types.
	 */
	template <typename U,
		  typename = typename std::enable_if<
			  !std::is_same<T, U>::value &&
			  std::is_same<typename std::remove_cv<T>::type,
				       U>::value>::type>
	persistent_ptr(persistent_ptr<U> const &r) noexcept
	    : persistent_ptr_base(r.oid)
	{
		this->oid.off +=
			static_cast<std::uint64_t>(calculate_offset<U>());
		verify_type();
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
	persistent_ptr(persistent_ptr<U> const &r) noexcept
	    : persistent_ptr_base(r.oid)
	{
		this->oid.off +=
			static_cast<std::uint64_t>(calculate_offset<U>());
		verify_type();
	}

	/*
	 * Operators
	 */

	/**
	 * Persistent pointer to void conversion operator.
	 */
	operator persistent_ptr<void>() const noexcept
	{
		return this->get();
	}

	/**
	 * Dereference operator.
	 */
	typename pmem::detail::sp_dereference<T>::type operator*() const
		noexcept
	{
		return *this->get();
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
	 * Prefix increment operator.
	 */
	inline persistent_ptr<T> &
	operator++()
	{
		detail::conditional_add_to_tx(this);
		this->oid.off += sizeof(T);

		return *this;
	}

	/**
	 * Postfix increment operator.
	 */
	inline persistent_ptr<T>
	operator++(int)
	{
		PMEMoid noid = this->oid;
		++(*this);

		return persistent_ptr<T>(noid);
	}

	/**
	 * Prefix decrement operator.
	 */
	inline persistent_ptr<T> &
	operator--()
	{
		detail::conditional_add_to_tx(this);
		this->oid.off -= sizeof(T);

		return *this;
	}

	/**
	 * Postfix decrement operator.
	 */
	inline persistent_ptr<T>
	operator--(int)
	{
		PMEMoid noid = this->oid;
		--(*this);

		return persistent_ptr<T>(noid);
	}

	/**
	 * Addition assignment operator.
	 */
	inline persistent_ptr<T> &
	operator+=(std::ptrdiff_t s)
	{
		detail::conditional_add_to_tx(this);
		this->oid.off += static_cast<std::uint64_t>(s) * sizeof(T);

		return *this;
	}

	/**
	 * Subtraction assignment operator.
	 */
	inline persistent_ptr<T> &
	operator-=(std::ptrdiff_t s)
	{
		detail::conditional_add_to_tx(this);
		this->oid.off -= static_cast<std::uint64_t>(s) * sizeof(T);

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
	persistent_ptr<T> &
	operator=(persistent_ptr<Y> const &r)
	{
		this_type(r).swap(*this);

		return *this;
	}

	/*
	 * Bool conversion operator.
	 */
	explicit operator bool() const noexcept
	{
		return get() != nullptr;
	}

	/*
	 * Persist/flush methods
	 */

	/**
	 * Persists the content of the underlying object.
	 *
	 * @param[in] pop Pmemobj pool
	 */
	void
	persist(pool_base &pop)
	{
		pop.persist(this->get(), sizeof(T));
	}

	/**
	 * Persists what the persistent pointer points to.
	 *
	 * @throw pool_error when cannot get pool from persistent
	 * pointer
	 */
	void
	persist(void)
	{
		pmemobjpool *pop = pmemobj_pool_by_oid(this->raw());

		if (pop == nullptr)
			throw pmem::pool_error(
				"Cannot get pool from persistent pointer");

		pmemobj_persist(pop, this->get(), sizeof(T));
	}

	/**
	 * Flushes what the persistent pointer points to.
	 *
	 * @param[in] pop Pmemobj pool
	 */
	void
	flush(pool_base &pop)
	{
		pop.flush(this->get(), sizeof(T));
	}

	/**
	 * Flushes what the persistent pointer points to.
	 *
	 * @throw pool_error when cannot get pool from persistent
	 * pointer
	 */
	void
	flush(void)
	{
		pmemobjpool *pop = pmemobj_pool_by_oid(this->raw());

		if (pop == nullptr)
			throw pmem::pool_error(
				"Cannot get pool from persistent pointer");

		pmemobj_flush(pop, this->get(), sizeof(T));
	}

	/*
	 * Pointer traits related.
	 */

	/**
	 * Create a persistent pointer from a given reference.
	 *
	 * This can create a persistent_ptr to a volatile object, use with
	 * extreme caution.
	 *
	 * @param ref reference to an object.
	 */
	static persistent_ptr<T>
	pointer_to(T &ref)
	{
		return persistent_ptr<T>(std::addressof(ref), 0);
	}

	/**
	 * Get the direct pointer.
	 *
	 * Performs a calculations on the underlying C-style pointer.
	 *
	 * @return the direct pointer to the object.
	 */
	element_type *
	get() const noexcept
	{
		if (this->oid.pool_uuid_lo ==
		    std::numeric_limits<decltype(oid.pool_uuid_lo)>::max())
			return reinterpret_cast<element_type *>(oid.off);
		else
			return static_cast<element_type *>(
				pmemobj_direct(this->oid));
	}

	/**
	 * Rebind to a different type of pointer.
	 */
	template <class U>
	using rebind = pmem::obj::persistent_ptr<U>;

	/**
	 * The persistency type to be used with this pointer.
	 */
	using persistency_type = p<T>;

	/**
	 * The used bool_type.
	 */
	using bool_type = bool;

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

	/**
	 * The pointer type.
	 */
	using pointer = persistent_ptr<T>;

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
	 * Private constructor enabling persistent_ptrs to volatile objects.
	 *
	 * This is internal implementation needed only for the
	 * pointer_traits<persistent_ptr>::pointer_to to be able to create
	 * valid pointers. This is used in libstdc++'s std::vector::insert().
	 */
	persistent_ptr(element_type *vptr, int) : persistent_ptr_base(vptr)
	{
		if (OID_IS_NULL(oid)) {
			oid.pool_uuid_lo = std::numeric_limits<decltype(
				oid.pool_uuid_lo)>::max();
			oid.off = reinterpret_cast<decltype(oid.off)>(vptr);
		}
	}

	/**
	 * Calculate in-object offset for structures with inheritance.
	 *
	 * In case of the given inheritance:
	 *
	 *	  A   B
	 *	   \ /
	 *	    C
	 *
	 * A pointer to B *ptr = &C should be offset by sizeof(A). This function
	 * calculates that offset.
	 *
	 * @return offset between two compatible pointer types to the same
	 * object
	 */
	template <typename U>
	inline ptrdiff_t
	calculate_offset() const
	{
		static const ptrdiff_t ptr_offset_magic = 0xDEADBEEF;

		U *tmp{reinterpret_cast<U *>(ptr_offset_magic)};
		T *diff = static_cast<T *>(tmp);
		return reinterpret_cast<ptrdiff_t>(diff) -
			reinterpret_cast<ptrdiff_t>(tmp);
	}
};

/**
 * Swaps two persistent_ptr objects of the same type.
 *
 * Non-member swap function as required by Swappable concept.
 * en.cppreference.com/w/cpp/concept/Swappable
 */
template <class T>
inline void
swap(persistent_ptr<T> &a, persistent_ptr<T> &b)
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
operator==(persistent_ptr<T> const &lhs, persistent_ptr<Y> const &rhs) noexcept
{
	return OID_EQUALS(lhs.raw(), rhs.raw());
}

/**
 * Inequality operator.
 */
template <typename T, typename Y>
inline bool
operator!=(persistent_ptr<T> const &lhs, persistent_ptr<Y> const &rhs) noexcept
{
	return !(lhs == rhs);
}

/**
 * Equality operator with nullptr.
 */
template <typename T>
inline bool
operator==(persistent_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return lhs.get() == nullptr;
}

/**
 * Equality operator with nullptr.
 */
template <typename T>
inline bool
operator==(std::nullptr_t, persistent_ptr<T> const &lhs) noexcept
{
	return lhs.get() == nullptr;
}

/**
 * Inequality operator with nullptr.
 */
template <typename T>
inline bool
operator!=(persistent_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return lhs.get() != nullptr;
}

/**
 * Inequality operator with nullptr.
 */
template <typename T>
inline bool
operator!=(std::nullptr_t, persistent_ptr<T> const &lhs) noexcept
{
	return lhs.get() != nullptr;
}

/**
 * Less than operator.
 *
 * @return true if the uuid_lo of lhs is less than the uuid_lo of rhs,
 * should they be equal, the offsets are compared. Returns false
 * otherwise.
 */
template <typename T, typename Y>
inline bool
operator<(persistent_ptr<T> const &lhs, persistent_ptr<Y> const &rhs) noexcept
{
	if (lhs.raw().pool_uuid_lo == rhs.raw().pool_uuid_lo)
		return lhs.raw().off < rhs.raw().off;
	else
		return lhs.raw().pool_uuid_lo < rhs.raw().pool_uuid_lo;
}

/**
 * Less or equal than operator.
 *
 * See less than operator for comparison rules.
 */
template <typename T, typename Y>
inline bool
operator<=(persistent_ptr<T> const &lhs, persistent_ptr<Y> const &rhs) noexcept
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
operator>(persistent_ptr<T> const &lhs, persistent_ptr<Y> const &rhs) noexcept
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
operator>=(persistent_ptr<T> const &lhs, persistent_ptr<Y> const &rhs) noexcept
{
	return !(lhs < rhs);
}

/* nullptr comparisons */

/**
 * Compare a persistent_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<(persistent_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return std::less<typename persistent_ptr<T>::element_type *>()(
		lhs.get(), nullptr);
}

/**
 * Compare a persistent_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<(std::nullptr_t, persistent_ptr<T> const &rhs) noexcept
{
	return std::less<typename persistent_ptr<T>::element_type *>()(
		nullptr, rhs.get());
}

/**
 * Compare a persistent_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<=(persistent_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return !(nullptr < lhs);
}

/**
 * Compare a persistent_ptr with a null pointer.
 */
template <typename T>
inline bool
operator<=(std::nullptr_t, persistent_ptr<T> const &rhs) noexcept
{
	return !(rhs < nullptr);
}

/**
 * Compare a persistent_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>(persistent_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return nullptr < lhs;
}

/**
 * Compare a persistent_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>(std::nullptr_t, persistent_ptr<T> const &rhs) noexcept
{
	return rhs < nullptr;
}

/**
 * Compare a persistent_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>=(persistent_ptr<T> const &lhs, std::nullptr_t) noexcept
{
	return !(lhs < nullptr);
}

/**
 * Compare a persistent_ptr with a null pointer.
 */
template <typename T>
inline bool
operator>=(std::nullptr_t, persistent_ptr<T> const &rhs) noexcept
{
	return !(nullptr < rhs);
}

/**
 * Addition operator for persistent pointers.
 */
template <typename T>
inline persistent_ptr<T>
operator+(persistent_ptr<T> const &lhs, std::ptrdiff_t s)
{
	PMEMoid noid;
	noid.pool_uuid_lo = lhs.raw().pool_uuid_lo;
	noid.off = lhs.raw().off + static_cast<std::uint64_t>(s) * sizeof(T);

	return persistent_ptr<T>(noid);
}

/**
 * Subtraction operator for persistent pointers.
 */
template <typename T>
inline persistent_ptr<T>
operator-(persistent_ptr<T> const &lhs, std::ptrdiff_t s)
{
	PMEMoid noid;
	noid.pool_uuid_lo = lhs.raw().pool_uuid_lo;
	noid.off = lhs.raw().off - static_cast<std::uint64_t>(s) * sizeof(T);

	return persistent_ptr<T>(noid);
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
operator-(persistent_ptr<T> const &lhs, persistent_ptr<Y> const &rhs)
{
	assert(lhs.raw().pool_uuid_lo == rhs.raw().pool_uuid_lo);
	auto d = static_cast<std::ptrdiff_t>(lhs.raw().off - rhs.raw().off);

	return d / static_cast<std::ptrdiff_t>(sizeof(T));
}

/**
 * Ostream operator for the persistent pointer.
 */
template <typename T>
std::ostream &
operator<<(std::ostream &os, persistent_ptr<T> const &pptr)
{
	PMEMoid raw_oid = pptr.raw();
	os << std::hex << "0x" << raw_oid.pool_uuid_lo << ", 0x" << raw_oid.off
	   << std::dec;
	return os;
}

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_PERSISTENT_PTR_HPP */
