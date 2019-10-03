//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018-2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/array.hpp>

template <int Dummy>
struct NoCompare {
};

int
main()
{
	START();

	int result = 0;
	{
		typedef NoCompare<2> T;
		typedef pmem::obj::array<T, 0> C;
		C c1 = {{}};
		// expected-error@algorithm:* 2 {{invalid operands to binary
		// expression}}
		result = (c1 == c1);
		result = (c1 < c1);
	}

	return result;
}
