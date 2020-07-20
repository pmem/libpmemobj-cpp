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
// Class helper to test pmem::obj containers
//

#ifndef LIBPMEMOBJ_CPP_TESTS_PRIVATE_CONSTRUCTOR_H
#define LIBPMEMOBJ_CPP_TESTS_PRIVATE_CONSTRUCTOR_H

#include <iostream>

#include "map_wrapper.hpp"

struct PrivateConstructor {

    PrivateConstructor static make ( int v ) { return PrivateConstructor(v); }
    int get () const { return val; }
private:
    PrivateConstructor ( int v ) : val(v) {}
    int val;
    };

bool operator < ( const PrivateConstructor &lhs, const PrivateConstructor &rhs ) { return lhs.get() < rhs.get(); }

bool operator < ( const PrivateConstructor &lhs, int rhs ) { return lhs.get() < rhs; }
bool operator < ( int lhs, const PrivateConstructor &rhs ) { return lhs < rhs.get(); }

std::ostream & operator << ( std::ostream &os, const PrivateConstructor &foo ) { return os << foo.get (); }

#ifdef LIBPMEMOBJ_CPP_TESTS_RADIX

struct pc_bytes_view {
    pc_bytes_view(const PrivateConstructor* pc): v((unsigned) (pc->get() + std::numeric_limits<int>::max() / 2)) {}

    size_t size() const {
        return sizeof(PrivateConstructor);
    }

    char operator[](std::ptrdiff_t p) const {
        return reinterpret_cast<const char*>(&v)[(ptrdiff_t)(size()) - p - 1];
    }

    unsigned v;
};

template <>
struct test_bytes_view<PrivateConstructor, void> {
    using type = pc_bytes_view;
};
#endif

#endif // LIBPMEMOBJ_CPP_TESTS_PRIVATE_CONSTRUCTOR_H
