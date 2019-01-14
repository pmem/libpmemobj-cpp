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

using pmem_exp::get;

int
main()
{
	START();

	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		C c = {{1, 2, 3.5}};
		get<3>(c) = 5.5; // expected-note {{requested here}}
		// expected-error@array:* {{static_assert failed "Index out of
		// bounds in std::get<> (std::array)"}}
	}

	return 0;
}
