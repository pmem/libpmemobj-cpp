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
#include <vector>

namespace pmem_exp = pmem::obj::experimental;

template <class Array>
void
test_compare(const Array &LHS, const Array &RHS)
{
	typedef std::vector<typename Array::value_type> Vector;
	const Vector LHSV(LHS.begin(), LHS.end());
	const Vector RHSV(RHS.begin(), RHS.end());
	UT_ASSERT((LHS == RHS) == (LHSV == RHSV));
	UT_ASSERT((LHS != RHS) == (LHSV != RHSV));
	UT_ASSERT((LHS < RHS) == (LHSV < RHSV));
	UT_ASSERT((LHS <= RHS) == (LHSV <= RHSV));
	UT_ASSERT((LHS > RHS) == (LHSV > RHSV));
	UT_ASSERT((LHS >= RHS) == (LHSV >= RHSV));
}

int
main()
{
	START();

	{
		typedef int T;
		typedef pmem_exp::array<T, 3> C;
		C c1 = {1, 2, 3};
		C c2 = {1, 2, 3};
		C c3 = {3, 2, 1};
		C c4 = {1, 2, 1};
		test_compare(c1, c2);
		test_compare(c1, c3);
		test_compare(c1, c4);
	}
	{
		typedef int T;
		typedef pmem_exp::array<T, 0> C;
		C c1 = {};
		C c2 = {};
		test_compare(c1, c2);
	}

	return 0;
}
