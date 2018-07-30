/*
Copyright 2016 Intel Corporation.  All Rights Reserved.

The source code contained or described herein and all documents related
to the source code ("Material") are owned by Intel Corporation or its
suppliers or licensors.  Title to the Material remains with Intel
Corporation or its suppliers and licensors.  The Material is protected
by worldwide copyright laws and treaty provisions.  No part of the
Material may be used, copied, reproduced, modified, published, uploaded,
posted, transmitted, distributed, or disclosed in any way without
Intel's prior express written permission.

No license under any patent, copyright, trade secret or other
intellectual property right is granted to or conferred upon you by
disclosure or delivery of the Materials, either expressly, by
implication, inducement, estoppel or otherwise.  Any license under such
intellectual property rights must be express and approved by Intel in
writing.
*/

#ifndef __TBB_persistent_pool_ptr_H
#define __TBB_persistent_pool_ptr_H

#include <cassert>
#include <type_traits>
#include <cstddef>

#include "libpmemobj++/detail/specialization.hpp"
#include "libpmemobj++/persistent_ptr.hpp"

namespace tbb {
namespace internal {

using namespace pmem;
using namespace pmem::obj;

template <typename T>
class persistent_pool_ptr {
    template <typename Y>
    friend class persistent_pool_ptr;

    typedef persistent_pool_ptr<T> this_type;

public:
    /**
    * Type of an actual object with all qualifier removed,
    * used for easy underlying type access
    */
    typedef typename pmem::detail::sp_element<T>::type element_type;

    persistent_pool_ptr() : off(0) {
        verify_type();
    }

    /**
    *  Default null constructor, zeroes the off.
    */
    persistent_pool_ptr( std::nullptr_t ) noexcept : off( 0 ) {
        verify_type();
    }

    /**
    * PMEMoid constructor.
    *
    * Provided for easy interoperability between C++ and C API's.
    *
    * @param oid C-style persistent pointer
    */
    persistent_pool_ptr( PMEMoid oid ) noexcept : off( oid.off ) {
        verify_type();
    }

    /**
    * PMEMoid constructor.
    *
    * Provided for easy interoperability between C++ and C API's.
    *
    * @param off offset inside persistent memory pool
    */
    persistent_pool_ptr( uint64_t _off ) noexcept : off( _off ) {
        verify_type();
    }

    /**
    * Copy constructor from a different persistent_pool_ptr<>.
    *
    * Available only for convertible types.
    *
    */
    template <typename Y,
        typename = typename std::enable_if<
        std::is_convertible<Y *, T *>::value>::type>
    persistent_pool_ptr( const persistent_pool_ptr<Y> &r ) noexcept : off( r.off )
    {
        verify_type();
    }

    /**
    * Copy constructor from a different persistent_ptr<>.
    *
    * Available only for convertible types.
    *
    */
    template <typename Y,
        typename = typename std::enable_if<
        std::is_convertible<Y *, T *>::value>::type>
    persistent_pool_ptr( const persistent_ptr<Y> &r ) noexcept : off( r.off )
    {
        verify_type();
    }

    /*
    * Copy constructor.
    *
    * @param r Persistent pool pointer to the same type.
    */
    persistent_pool_ptr( const persistent_pool_ptr &r ) noexcept : off( r.off )
    {
        verify_type();
    }

    /*
    * Copy constructor from a persistent_ptr.
    *
    * @param r Persistent pointer to the same type.
    */
    persistent_pool_ptr( const persistent_ptr<T> &r ) noexcept : off( r.raw().off )
    {
        verify_type();
    }

    /**
    * Defaulted move constructor.
    */
    persistent_pool_ptr( persistent_pool_ptr &&r ) noexcept : off( std::move( r.off ) )
    {
        verify_type();
    }

    /**
    * Defaulted move assignment operator.
    */
    persistent_pool_ptr &
        operator=( persistent_pool_ptr &&r )
    {
        detail::conditional_add_to_tx( this );
        this->off = std::move( r.off );

        return *this;
    }

    persistent_pool_ptr &
        operator=( std::nullptr_t nullp )
    {
        detail::conditional_add_to_tx( this );
        this->off = 0;

        return *this;
    }

    /**
    * Assignment operator.
    *
    * Persistent pool pointer assignment within a transaction
    * automatically registers this operation so that a rollback
    * is possible.
    *
    * @throw pmem::transaction_error when adding the object to the
    *	transaction failed.
    */
    persistent_pool_ptr &
        operator=( const persistent_pool_ptr &r )
    {
        detail::conditional_add_to_tx( this );
        this->off = r.off;

        return *this;
    }

    /**
    * Assignment operator from a persistent_ptr.
    *
    * Persistent pool pointer assignment within a transaction
    * automatically registers this operation so that a rollback
    * is possible.
    *
    * @throw pmem::transaction_error when adding the object to the
    *	transaction failed.
    */
    persistent_pool_ptr &
        operator=( const persistent_ptr<T> &r )
    {
        detail::conditional_add_to_tx( this );
        this->off = r.raw().off;

        return *this;
    }

    /**
    * Assignment operator from a PMEMoid.
    *
    * Persistent pool pointer assignment within a transaction
    * automatically registers this operation so that a rollback
    * is possible.
    *
    * @throw pmem::transaction_error when adding the object to the
    *	transaction failed.
    */
    persistent_pool_ptr &
        operator=( const PMEMoid &oid )
    {
        detail::conditional_add_to_tx( this );
        this->off = oid.off;
        return *this;
    }

    /**
    * Converting assignment operator from a different
    * persistent_pool_ptr<>.
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
        std::is_convertible<Y *, T *>::value>::type>
    persistent_pool_ptr &
        operator=( const persistent_pool_ptr<Y> &r )
    {
        detail::conditional_add_to_tx( this );
        this->off = r.off;

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
    * @throw pmem::transaction_error when adding the object to the
    *	transaction failed.
    */
    template <typename Y,
        typename = typename std::enable_if<
        std::is_convertible<Y *, T *>::value>::type>
        persistent_pool_ptr &
        operator=( const persistent_ptr<Y> &r )
    {
        detail::conditional_add_to_tx( this );
        this->off = r.raw().off;

        return *this;
    }

    /**
    * Get a direct pointer.
    *
    * Performs a calculations on the underlying C-style pointer.
    *
    * @return a direct pointer to the object.
    */
    element_type* get( uint64_t pool_uuid ) const noexcept {
        PMEMoid oid = { pool_uuid, this->off };
        return (element_type *)pmemobj_direct( oid );
    }

    element_type* operator()( uint64_t pool_uuid ) const noexcept {
        return get( pool_uuid );
    }

    /**
    * Get a persistent pointer.
    *
    * Performs a calculations on the underlying C-style pointer.
    *
    * @return a direct pointer to the object.
    */
    persistent_ptr<T> get_persistent_ptr( uint64_t pool_uuid ) const noexcept {
        PMEMoid oid = { pool_uuid, this->off };
        return persistent_ptr<T>( oid );
    }

    /**
    * Swaps two persistent_pool_ptr objects of the same type.
    */
    void swap( persistent_pool_ptr &other ) noexcept {
        std::swap( this->off, other.off );
    }

    /*
    * Bool conversion operator.
    */
    explicit operator bool() const noexcept
    {
        return this->off != 0;
    }

    /**
    * Get PMEMoid encapsulated by this object.
    *
    * For C API compatibility.
    *
    * @return const reference to the PMEMoid
    */
    PMEMoid raw_oid( uint64_t pool_uuid) const noexcept
    {
        PMEMoid oid = { pool_uuid, this->off };
        return oid;
    }

    const uint64_t& raw() const noexcept
    {
        return this->off;
    }

    uint64_t& raw()
    {
        detail::conditional_add_to_tx( this );
        return this->off;
    }

    /**
    * Prefix increment operator.
    */
    inline persistent_pool_ptr<T> &operator++()
    {
        detail::conditional_add_to_tx( this );
        this->off += sizeof( T );

        return *this;
    }

    /**
    * Postfix increment operator.
    */
    inline persistent_pool_ptr<T> operator++( int )
    {
        persistent_pool_ptr<T> ret( *this );
        ++(*this);

        return ret;
    }

    /**
    * Prefix decrement operator.
    */
    inline persistent_pool_ptr<T> &operator--()
    {
        detail::conditional_add_to_tx( this );
        this->off -= sizeof( T );

        return *this;
    }

    /**
    * Postfix decrement operator.
    */
    inline persistent_pool_ptr<T> operator--( int )
    {
        persistent_pool_ptr<T> ret( *this );
        --(*this);

        return ret;
    }

    /**
    * Addition assignment operator.
    */
    inline persistent_pool_ptr<T> &
        operator+=( std::ptrdiff_t s )
    {
        detail::conditional_add_to_tx( this );
        this->off += s * sizeof( T );

        return *this;
    }

    /**
    * Subtraction assignment operator.
    */
    inline persistent_pool_ptr<T> &
        operator-=( std::ptrdiff_t s )
    {
        detail::conditional_add_to_tx( this );
        this->off -= s * sizeof( T );

        return *this;
    }

    inline persistent_pool_ptr<T> operator+( std::ptrdiff_t s ) {
        persistent_pool_ptr<T> ret( *this );
        ret.off += s * sizeof( T );

        return ret;
    }

    inline persistent_pool_ptr<T> operator-( std::ptrdiff_t s ) {
        persistent_pool_ptr<T> ret( *this );
        ret.off -= s * sizeof( T );

        return ret;
    }

private:
    /* offset of persistent object in a persistent memory pool*/
    uint64_t off;

    void verify_type()
    {
        static_assert(!std::is_polymorphic<element_type>::value,
            "Polymorphic types are not supported");
    }
};

/**
* Equality operator.
*
* This checks if underlying PMEMoids are equal.
*/
template <typename T, typename Y>
inline bool
operator==( const persistent_pool_ptr<T> &lhs, const persistent_pool_ptr<Y> &rhs ) noexcept
{
    return lhs.raw() == rhs.raw();
}

/**
* Inequality operator.
*/
template <typename T, typename Y>
inline bool
operator!=( const persistent_pool_ptr<T> &lhs, const persistent_pool_ptr<Y> &rhs ) noexcept
{
    return !(lhs == rhs);
}

/**
* Inequality operator with nullptr.
*/
template <typename T>
inline bool
operator!=( const persistent_pool_ptr<T> &lhs, std::nullptr_t ) noexcept
{
    return lhs.raw() != 0;
}

/**
* Equality operator with nullptr.
*/
template <typename T>
inline bool
operator==( const persistent_pool_ptr<T> &lhs, std::nullptr_t ) noexcept
{
    return lhs.raw() == 0;
}

/**
* Equality operator with nullptr.
*/
template <typename T>
inline bool
operator==( std::nullptr_t, const persistent_pool_ptr<T> &lhs ) noexcept
{
    return lhs.raw() == 0;
}

template<class T, class U>
persistent_pool_ptr<T> static_persistent_pool_pointer_cast( const persistent_pool_ptr<U>& r ) {
    static_cast<T*>( (U*)0 );
    return persistent_pool_ptr<T>( r.raw() );
}

} // namespace internal
} // namespace tbb

#endif // __TBB_persistent_pool_ptr_H
