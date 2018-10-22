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

struct NoDefault {
	NoDefault(int)
	{
	}
};

int
main()
{
	START();

	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		C c;
		UT_ASSERT(c.size() == 3);
	}
	{
		typedef double T;
		typedef pmem_exp::array<T, 0> C;
		C c;
		UT_ASSERT(c.size() == 0);
	}
	{
		typedef pmem_exp::array<NoDefault, 0> C;
		C c;
		UT_ASSERT(c.size() == 0);
		C c1 = {};
		UT_ASSERT(c1.size() == 0);
		C c2 = {{}};
		UT_ASSERT(c2.size() == 0);
	}

	return 0;
}
