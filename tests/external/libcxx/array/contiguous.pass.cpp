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

template <class C>
void
test_contiguous(const C &c)
{
	for (size_t i = 0; i < c.size(); ++i)
		UT_ASSERT(*(c.begin() + (ptrdiff_t)i) ==
			  *(std::addressof(*c.begin()) + (ptrdiff_t)i));
}

int
main()
{
	START();

	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		test_contiguous(C());
	}

	return 0;
}
