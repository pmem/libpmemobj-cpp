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
		C::reference r1 = c[0];
		UT_ASSERT(r1 == 1);
		r1 = 5.5;
		UT_ASSERT(c.front() == 5.5);

		C::reference r2 = c[2];
		UT_ASSERT(r2 == 3.5);
		r2 = 7.5;
		UT_ASSERT(c.back() == 7.5);
	}
	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		const C c = {1, 2, 3.5};
		C::const_reference r1 = c[0];
		UT_ASSERT(r1 == 1);
		C::const_reference r2 = c[2];
		UT_ASSERT(r2 == 3.5);
	}
	{ // Test operator[] "works" on zero sized arrays
		typedef double T;
		typedef pmem_exp::array<T, 0> C;
		C c = {};
		C const &cc = c;
		static_assert((std::is_same<decltype(c[0]), T &>::value), "");
		static_assert((std::is_same<decltype(cc[0]), const T &>::value),
			      "");
		if (c.size() > (0)) { // always false
			C::reference r1 = c[0];
			C::const_reference r2 = cc[0];
			((void)r1);
			((void)r2);
		}
	}
	{ // Test operator[] "works" on zero sized arrays
		typedef double T;
		typedef pmem_exp::array<const T, 0> C;
		C c = {{}};
		C const &cc = c;
		static_assert((std::is_same<decltype(c[0]), const T &>::value),
			      "");
		static_assert((std::is_same<decltype(cc[0]), const T &>::value),
			      "");
		if (c.size() > (0)) { // always false
			C::reference r1 = c[0];
			C::const_reference r2 = cc[0];
			((void)r1);
			((void)r2);
		}
	}

	return 0;
}
