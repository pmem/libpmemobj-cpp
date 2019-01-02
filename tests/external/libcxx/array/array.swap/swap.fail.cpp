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

using pmem_exp::swap;

int
main()
{
	START();

	{

		typedef double T;
		typedef pmem_exp::array<const T, 0> C;
		C c = {{}};
		C c2 = {{}};
		// expected-error-re@array:* {{static_assert failed
		// {{.*}}"cannot swap zero-sized array of type 'const T'"}}
		c.swap(c2); // expected-note {{requested here}}
	}

	return 0;
}
