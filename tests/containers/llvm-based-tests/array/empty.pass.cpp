//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018, Intel Corporation
//
// Modified to use with libpmemobj-cpp
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/array.hpp>

namespace pmem_exp = pmem::obj::experimental;

int
main()
{
	{
		typedef pmem_exp::array<int, 2> C;
		C c;
		UT_ASSERT_NOEXCEPT(c.empty());
		UT_ASSERT(!c.empty());
	}
	{
		typedef pmem_exp::array<int, 0> C;
		C c;
		UT_ASSERT_NOEXCEPT(c.empty());
		UT_ASSERT(c.empty());
	}
}
