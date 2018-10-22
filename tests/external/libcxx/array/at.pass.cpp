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
		C::reference r1 = c.at(0);
		UT_ASSERT(r1 == 1);
		r1 = 5.5;
		UT_ASSERT(c.front() == 5.5);

		C::reference r2 = c.at(2);
		UT_ASSERT(r2 == 3.5);
		r2 = 7.5;
		UT_ASSERT(c.back() == 7.5);

		try {
			c.at(3);
			UT_ASSERT(false);
		} catch (const std::out_of_range &) {
		}
	}

	{
		typedef double T;
		typedef pmem_exp::array<T, 0> C;
		C c = {};
		C const &cc = c;
		try {
			c.at(0);
			UT_ASSERT(false);
		} catch (const std::out_of_range &) {
		}
		try {
			cc.at(0);
			UT_ASSERT(false);
		} catch (const std::out_of_range &) {
		}
	}

	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		const C c = {1, 2, 3.5};
		C::const_reference r1 = c.at(0);
		UT_ASSERT(r1 == 1);

		C::const_reference r2 = c.at(2);
		UT_ASSERT(r2 == 3.5);

		try {
			c.at(3);
			UT_ASSERT(false);
		} catch (const std::out_of_range &) {
		}
	}

	return 0;
}
