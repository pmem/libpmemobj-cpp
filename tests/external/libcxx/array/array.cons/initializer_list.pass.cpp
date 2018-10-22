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
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		C c = {1, 2, 3.5};
		UT_ASSERT(c.size() == 3);
		UT_ASSERT(c[0] == 1);
		UT_ASSERT(c[1] == 2);
		UT_ASSERT(c[2] == 3.5);
	}
	{
		typedef double T;
		typedef pmem_exp::array<T, 0> C;
		C c = {};
		UT_ASSERT(c.size() == 0);
	}

	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		C c = {1};
		UT_ASSERT(c.size() == 3.0);
		UT_ASSERT(c[0] == 1);
	}
	{
		typedef int T;
		typedef pmem_exp::array<T, 1> C;
		C c = {};
		UT_ASSERT(c.size() == 1);
	}

	return 0;
}
