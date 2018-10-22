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

struct NoDefault {
	NoDefault(int)
	{
	}
};

int
main()
{
	START();

	{
		typedef double T;
		typedef pmem_exp::array<T, 3> C;
		C c = {{1.1, 2.2, 3.3}};
		C c2 = c;
		c2 = c;
		static_assert(std::is_copy_constructible<C>::value, "");
		static_assert(std::is_copy_assignable<C>::value, "");
	}
	{
		typedef double T;
		typedef pmem_exp::array<const T, 3> C;
		C c = {{1.1, 2.2, 3.3}};
		C c2 = c;
		((void)c2);
		static_assert(std::is_copy_constructible<C>::value, "");
	}
	{
		typedef double T;
		typedef pmem_exp::array<T, 0> C;
		C c = {{}};
		C c2 = c;
		c2 = c;
		static_assert(std::is_copy_constructible<C>::value, "");
		static_assert(std::is_copy_assignable<C>::value, "");
	}
	{
		// const arrays of size 0 should disable the implicit copy
		// assignment operator.
		typedef double T;
		typedef pmem_exp::array<const T, 0> C;
		C c = {{}};
		C c2 = c;
		((void)c2);
		static_assert(std::is_copy_constructible<C>::value, "");
	}
	{
		typedef NoDefault T;
		typedef pmem_exp::array<T, 0> C;
		C c = {{}};
		C c2 = c;
		c2 = c;
		static_assert(std::is_copy_constructible<C>::value, "");
		static_assert(std::is_copy_assignable<C>::value, "");
	}
	{
		typedef NoDefault T;
		typedef pmem_exp::array<const T, 0> C;
		C c = {{}};
		C c2 = c;
		((void)c2);
		static_assert(std::is_copy_constructible<C>::value, "");
	}

	return 0;
}
