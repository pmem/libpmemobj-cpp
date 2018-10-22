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

		C::reference r1 = c.front();
		UT_ASSERT(r1 == 1);
		r1 = 5.5;
		UT_ASSERT(c[0] == 5.5);

		C::reference r2 = c.back();
		UT_ASSERT(r2 == 3.5);
		r2 = 7.5;
		UT_ASSERT(c[2] == 7.5);
	}
	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		const C c = {1, 2, 3.5};
		C::const_reference r1 = c.front();
		UT_ASSERT(r1 == 1);

		C::const_reference r2 = c.back();
		UT_ASSERT(r2 == 3.5);
	}
	{
		typedef double T;
		typedef pmem_exp::array<T, 0> C;
		C c = {};
		C const &cc = c;
		static_assert((std::is_same<decltype(c.front()), T &>::value),
			      "");
		static_assert(
			(std::is_same<decltype(cc.front()), const T &>::value),
			"");
		static_assert((std::is_same<decltype(c.back()), T &>::value),
			      "");
		static_assert(
			(std::is_same<decltype(cc.back()), const T &>::value),
			"");
		if (c.size() > (0)) { // always false
			c.front();
			c.back();
			cc.front();
			cc.back();
		}
	}
	{
		typedef double T;
		typedef pmem_exp::array<const T, 0> C;
		C c = {{}};
		C const &cc = c;
		static_assert(
			(std::is_same<decltype(c.front()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(cc.front()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(c.back()), const T &>::value),
			"");
		static_assert(
			(std::is_same<decltype(cc.back()), const T &>::value),
			"");
		if (c.size() > (0)) {
			c.front();
			c.back();
			cc.front();
			cc.back();
		}
	}

	return 0;
}
