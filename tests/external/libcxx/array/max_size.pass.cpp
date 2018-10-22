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
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/array.hpp>

namespace pmem_exp = pmem::obj::experimental;

int
main()
{
	START();

	{
		typedef pmem_exp::array<int, 2> C;
		C c;
		UT_ASSERT_NOEXCEPT(c.max_size());
		UT_ASSERT(c.max_size() == 2);
	}
	{
		typedef pmem_exp::array<int, 0> C;
		C c;
		UT_ASSERT_NOEXCEPT(c.max_size());
		UT_ASSERT(c.max_size() == 0);
	}

	return 0;
}
