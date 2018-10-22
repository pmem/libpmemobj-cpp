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
		C::iterator i;
		i = c.begin();
		UT_ASSERT(*i == 1);
		UT_ASSERT(&*i == c.data());
		*i = 5.5;
		UT_ASSERT(c[0] == 5.5);
	}
	{
		struct NoDefault {
			NoDefault(int)
			{
			}
		};
		typedef NoDefault T;
		typedef pmem_exp::array<T, 0> C;
		C c = {};
		UT_ASSERT(c.begin() == c.end());
	}

	return 0;
}
