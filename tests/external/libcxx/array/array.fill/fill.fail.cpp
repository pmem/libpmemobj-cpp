//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/array.hpp>

int
main()
{
	{
		typedef double T;
		typedef pmem::obj::array<const T, 0> C;
		C c = {{}};
		// expected-error-re@array:* {{static_assert failed
		// {{.*}}"cannot fill zero-sized array of type 'const T'"}}
		c.fill(5.5); // expected-note {{requested here}}
	}

	return 0;
}
