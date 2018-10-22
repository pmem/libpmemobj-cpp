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
#include <cstddef>

#include <libpmemobj++/experimental/array.hpp>

namespace pmem_exp = pmem::obj::experimental;

int
main()
{
	START();

	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		const C c = {1, 2, 3.5};
		const T *p = c.data();
		UT_ASSERT(p[0] == 1);
		UT_ASSERT(p[1] == 2);
		UT_ASSERT(p[2] == 3.5);
	}
	{
		typedef std::max_align_t T;
		typedef pmem_exp::array<T, 0> C;
		const C c = {};
		const T *p = c.data();
		std::uintptr_t pint = reinterpret_cast<std::uintptr_t>(p);
		UT_ASSERT(pint % alignof(std::max_align_t) == 0);
	}

	return 0;
}
