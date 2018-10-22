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

struct NonSwappable {
	NonSwappable()
	{
	}

private:
	NonSwappable(NonSwappable const &);
	NonSwappable &operator=(NonSwappable const &);
};

int
main()
{
	START();

	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		C c1 = {1, 2, 3.5};
		C c2 = {4, 5, 6.5};
		c1.swap(c2);
		UT_ASSERT(c1.size() == 3);
		UT_ASSERT(c1[0] == 4);
		UT_ASSERT(c1[1] == 5);
		UT_ASSERT(c1[2] == 6.5);
		UT_ASSERT(c2.size() == 3);
		UT_ASSERT(c2[0] == 1);
		UT_ASSERT(c2[1] == 2);
		UT_ASSERT(c2[2] == 3.5);
	}
	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		C c1 = {1, 2, 3.5};
		C c2 = {4, 5, 6.5};
		swap(c1, c2);
		UT_ASSERT(c1.size() == 3);
		UT_ASSERT(c1[0] == 4);
		UT_ASSERT(c1[1] == 5);
		UT_ASSERT(c1[2] == 6.5);
		UT_ASSERT(c2.size() == 3);
		UT_ASSERT(c2[0] == 1);
		UT_ASSERT(c2[1] == 2);
		UT_ASSERT(c2[2] == 3.5);
	}

	{
		typedef double T;
		typedef pmem_exp::array<T, 0> C;
		C c1 = {};
		C c2 = {};
		c1.swap(c2);
		UT_ASSERT(c1.size() == 0);
		UT_ASSERT(c2.size() == 0);
	}
	{
		typedef double T;
		typedef pmem_exp::array<T, 0> C;
		C c1 = {};
		C c2 = {};
		swap(c1, c2);
		UT_ASSERT(c1.size() == 0);
		UT_ASSERT(c2.size() == 0);
	}
	{
		typedef NonSwappable T;
		typedef pmem_exp::array<T, 0> C0;
		C0 l = {};
		C0 r = {};
		l.swap(r);
	}

	return 0;
}
