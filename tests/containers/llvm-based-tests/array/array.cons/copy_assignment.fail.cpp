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
		typedef double T;
		typedef pmem_exp::array<const T, 3> C;
		C c = {{1.1, 2.2, 3.3}}, c2 = {{1, 2, 3}};
		c2 = c;
	}
}
