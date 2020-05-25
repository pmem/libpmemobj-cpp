//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Copyright 2020, Intel Corporation
//
// testing transparent
//

#ifndef LIBPMEMOBJ_CPP_TESTS_TRANSPARENT_H
#define LIBPMEMOBJ_CPP_TESTS_TRANSPARENT_H

#include "unittest.hpp"

struct transparent_less
{
    template <class T, class U>
    constexpr auto operator()(T&& t, U&& u) const
    noexcept(noexcept(std::forward<T>(t) < std::forward<U>(u)))
    -> decltype      (std::forward<T>(t) < std::forward<U>(u))
        { return      std::forward<T>(t) < std::forward<U>(u); }
    using is_transparent = void;  // correct
};

struct transparent_less_not_referenceable
{
    template <class T, class U>
    constexpr auto operator()(T&& t, U&& u) const
    noexcept(noexcept(std::forward<T>(t) < std::forward<U>(u)))
    -> decltype      (std::forward<T>(t) < std::forward<U>(u))
        { return      std::forward<T>(t) < std::forward<U>(u); }
    using is_transparent = void () const &;  // it's a type; a weird one, but a type
};

struct C2Int { // comparable to int
    C2Int() : i_(0) {}
    C2Int(int i): i_(i) {}
    int get () const { return i_; }
private:
    int i_;
    };

bool operator <(int          rhs,   const C2Int& lhs) { return rhs       < lhs.get(); }
bool operator <(const C2Int& rhs,   const C2Int& lhs) { return rhs.get() < lhs.get(); }
bool operator <(const C2Int& rhs,            int lhs) { return rhs.get() < lhs; }

#endif  // LIBPMEMOBJ_CPP_TESTS_TRANSPARENT_H
