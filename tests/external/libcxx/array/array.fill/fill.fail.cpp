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
	{
		START();

		typedef double T;
		typedef pmem_exp::array<const T, 0> C;
		C c = {{}};
		// expected-error-re@array:* {{static_assert failed
		// {{.*}}"cannot fill zero-sized array of type 'const T'"}}
		c.fill(5.5); // expected-note {{requested here}}
	}

	return 0;
}
