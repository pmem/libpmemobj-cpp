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

struct NonSwappable {
	NonSwappable()
	{
	}

private:
	NonSwappable(NonSwappable const &);
	NonSwappable &operator=(NonSwappable const &);
};

using pmem_exp::swap;

template <class Tp>
decltype(swap(std::declval<Tp>(), std::declval<Tp>())) can_swap_imp(int);

template <class Tp>
std::false_type can_swap_imp(...);

template <class Tp>
struct can_swap : std::is_same<decltype(can_swap_imp<Tp>(0)), void> {
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
		swap(c1, c2);
		UT_ASSERT(c1.size() == 0);
		UT_ASSERT(c2.size() == 0);
	}
	{
		typedef NonSwappable T;
		typedef pmem_exp::array<T, 0> C0;
		static_assert(can_swap<C0 &>::value, "");
		C0 l = {};
		C0 r = {};
		swap(l, r);
	}

	return 0;
}
