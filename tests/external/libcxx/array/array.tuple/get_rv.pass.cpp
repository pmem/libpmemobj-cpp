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
		typedef std::unique_ptr<double> T;
		typedef pmem_exp::array<T, 1> C;
		C c = {std::unique_ptr<double>(new double(3.5))};
		T t = get<0>(std::move(c));
		UT_ASSERT(*t == 3.5);
	}

	return 0;
}
