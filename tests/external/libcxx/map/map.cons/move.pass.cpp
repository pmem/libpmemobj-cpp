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
// Modified to test pmem::obj containers
//

// UNSUPPORTED: c++98, c++03

// <map>

// class map

// map(map&& m);

#include <map>
#include <cassert>

#include "test_macros.h"
#include "../../../test_compare.h"
#include "test_allocator.h"
#include "min_allocator.h"

int main(int, char**)
{
#ifdef XXX // XXX: Implement test_allocator and min_allocator
    typedef std::pair<const int, double> V;
    {
        typedef test_compare<std::less<int> > C;
        typedef test_allocator<V> A;
        std::map<int, double, C, A> mo(C(5), A(7));
        std::map<int, double, C, A> m = std::move(mo);
        UT_ASSERT(m.get_allocator() == A(7));
        UT_ASSERT(m.key_comp() == C(5));
        UT_ASSERT(m.size() == 0);
        UT_ASSERT(distance(m.begin(), m.end()) == 0);

        UT_ASSERT(mo.get_allocator() == A(test_alloc_base::moved_value));
        UT_ASSERT(mo.key_comp() == C(5));
        UT_ASSERT(mo.size() == 0);
        UT_ASSERT(distance(mo.begin(), mo.end()) == 0);
    }
    {
        V ar[] =
        {
            V(1, 1),
            V(1, 1.5),
            V(1, 2),
            V(2, 1),
            V(2, 1.5),
            V(2, 2),
            V(3, 1),
            V(3, 1.5),
            V(3, 2),
        };
        typedef test_compare<std::less<int> > C;
        typedef test_allocator<V> A;
        std::map<int, double, C, A> mo(ar, ar+sizeof(ar)/sizeof(ar[0]), C(5), A(7));
        std::map<int, double, C, A> m = std::move(mo);
        UT_ASSERT(m.get_allocator() == A(7));
        UT_ASSERT(m.key_comp() == C(5));
        UT_ASSERT(m.size() == 3);
        UT_ASSERT(distance(m.begin(), m.end()) == 3);
        UT_ASSERT(*m.begin() == V(1, 1));
        UT_ASSERT(*next(m.begin()) == V(2, 1));
        UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

        UT_ASSERT(mo.get_allocator() == A(test_alloc_base::moved_value));
        UT_ASSERT(mo.key_comp() == C(5));
        UT_ASSERT(mo.size() == 0);
        UT_ASSERT(distance(mo.begin(), mo.end()) == 0);
    }
    {
        typedef test_compare<std::less<int> > C;
        typedef min_allocator<V> A;
        std::map<int, double, C, A> mo(C(5), A());
        std::map<int, double, C, A> m = std::move(mo);
        UT_ASSERT(m.get_allocator() == A());
        UT_ASSERT(m.key_comp() == C(5));
        UT_ASSERT(m.size() == 0);
        UT_ASSERT(distance(m.begin(), m.end()) == 0);

        UT_ASSERT(mo.get_allocator() == A());
        UT_ASSERT(mo.key_comp() == C(5));
        UT_ASSERT(mo.size() == 0);
        UT_ASSERT(distance(mo.begin(), mo.end()) == 0);
    }
    {
        V ar[] =
        {
            V(1, 1),
            V(1, 1.5),
            V(1, 2),
            V(2, 1),
            V(2, 1.5),
            V(2, 2),
            V(3, 1),
            V(3, 1.5),
            V(3, 2),
        };
        typedef test_compare<std::less<int> > C;
        typedef min_allocator<V> A;
        std::map<int, double, C, A> mo(ar, ar+sizeof(ar)/sizeof(ar[0]), C(5), A());
        std::map<int, double, C, A> m = std::move(mo);
        UT_ASSERT(m.get_allocator() == A());
        UT_ASSERT(m.key_comp() == C(5));
        UT_ASSERT(m.size() == 3);
        UT_ASSERT(distance(m.begin(), m.end()) == 3);
        UT_ASSERT(*m.begin() == V(1, 1));
        UT_ASSERT(*next(m.begin()) == V(2, 1));
        UT_ASSERT(*next(m.begin(), 2) == V(3, 1));

        UT_ASSERT(mo.get_allocator() == A());
        UT_ASSERT(mo.key_comp() == C(5));
        UT_ASSERT(mo.size() == 0);
        UT_ASSERT(distance(mo.begin(), mo.end()) == 0);
    }
#endif

  return 0;
}
