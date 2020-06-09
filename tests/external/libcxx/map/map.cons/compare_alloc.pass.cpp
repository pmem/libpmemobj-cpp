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

// <map>

// class map

// map(const key_compare& comp, const allocator_type& a);

#include <map>
#include <cassert>

#include "test_macros.h"
#include "../../../test_compare.h"
#include "test_allocator.h"
#include "min_allocator.h"

int main(int, char**)
{
    {
    typedef test_compare<std::less<int> > C;
    typedef test_allocator<std::pair<const int, double> > A;
    std::map<int, double, C, A> m(C(4), A(5));
    UT_ASSERT(m.empty());
    UT_ASSERT(m.begin() == m.end());
    UT_ASSERT(m.key_comp() == C(4));
    UT_ASSERT(m.get_allocator() == A(5));
    }
#ifdef XXX
    {
    typedef test_compare<std::less<int> > C;
    typedef min_allocator<std::pair<const int, double> > A;
    std::map<int, double, C, A> m(C(4), A());
    UT_ASSERT(m.empty());
    UT_ASSERT(m.begin() == m.end());
    UT_ASSERT(m.key_comp() == C(4));
    UT_ASSERT(m.get_allocator() == A());
    }
    {
    typedef test_compare<std::less<int> > C;
    typedef explicit_allocator<std::pair<const int, double> > A;
    std::map<int, double, C, A> m(C(4), A{});
    UT_ASSERT(m.empty());
    UT_ASSERT(m.begin() == m.end());
    UT_ASSERT(m.key_comp() == C(4));
    UT_ASSERT(m.get_allocator() == A{});
    }
#endif

  return 0;
}
