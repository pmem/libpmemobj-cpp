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
		typedef pmem_exp::array<int, 5> C;
		C c;
		C::iterator i;
		i = c.begin();
		C::const_iterator j;
		j = c.cbegin();
		UT_ASSERT(i == j);
	}
	{
		typedef pmem_exp::array<int, 0> C;
		C c;
		C::iterator i;
		i = c.begin();
		C::const_iterator j;
		j = c.cbegin();
		UT_ASSERT(i == j);
	}

	{
		typedef pmem_exp::array<int, 5> C;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);
		UT_ASSERT(ii1 == cii);

		UT_ASSERT(!(ii1 != ii2));
		UT_ASSERT(!(ii1 != cii));

		C c;
		UT_ASSERT(c.begin() == pmem_exp::begin(c));
		UT_ASSERT(c.cbegin() == pmem_exp::cbegin(c));
		UT_ASSERT(c.rbegin() == pmem_exp::rbegin(c));
		UT_ASSERT(c.crbegin() == pmem_exp::crbegin(c));
		UT_ASSERT(c.end() == pmem_exp::end(c));
		UT_ASSERT(c.cend() == pmem_exp::cend(c));
		UT_ASSERT(c.rend() == pmem_exp::rend(c));
		UT_ASSERT(c.crend() == pmem_exp::crend(c));

		UT_ASSERT(pmem_exp::begin(c) != pmem_exp::end(c));
		UT_ASSERT(pmem_exp::rbegin(c) != pmem_exp::rend(c));
		UT_ASSERT(pmem_exp::cbegin(c) != pmem_exp::cend(c));
		UT_ASSERT(pmem_exp::crbegin(c) != pmem_exp::crend(c));
	}
	{
		typedef pmem_exp::array<int, 0> C;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);

		UT_ASSERT(!(ii1 != ii2));

		UT_ASSERT((ii1 == cii));
		UT_ASSERT((cii == ii1));
		UT_ASSERT(!(ii1 != cii));
		UT_ASSERT(!(cii != ii1));
		UT_ASSERT(!(ii1 < cii));
		UT_ASSERT(!(cii < ii1));
		UT_ASSERT((ii1 <= cii));
		UT_ASSERT((cii <= ii1));
		UT_ASSERT(!(ii1 > cii));
		UT_ASSERT(!(cii > ii1));
		UT_ASSERT((ii1 >= cii));
		UT_ASSERT((cii >= ii1));
		UT_ASSERT(cii - ii1 == 0);
		UT_ASSERT(ii1 - cii == 0);

		C c;
		UT_ASSERT(c.begin() == pmem_exp::begin(c));
		UT_ASSERT(c.cbegin() == pmem_exp::cbegin(c));
		UT_ASSERT(c.rbegin() == pmem_exp::rbegin(c));
		UT_ASSERT(c.crbegin() == pmem_exp::crbegin(c));
		UT_ASSERT(c.end() == pmem_exp::end(c));
		UT_ASSERT(c.cend() == pmem_exp::cend(c));
		UT_ASSERT(c.rend() == pmem_exp::rend(c));
		UT_ASSERT(c.crend() == pmem_exp::crend(c));

		UT_ASSERT(pmem_exp::begin(c) == pmem_exp::end(c));
		UT_ASSERT(pmem_exp::rbegin(c) == pmem_exp::rend(c));
		UT_ASSERT(pmem_exp::cbegin(c) == pmem_exp::cend(c));
		UT_ASSERT(pmem_exp::crbegin(c) == pmem_exp::crend(c));
	}

	{
		typedef pmem_exp::array<int, 5> C;
		C c{{0, 1, 2, 3, 4}};

		UT_ASSERT(c.begin() == pmem_exp::begin(c));
		UT_ASSERT(c.cbegin() == pmem_exp::cbegin(c));
		UT_ASSERT(c.end() == pmem_exp::end(c));
		UT_ASSERT(c.cend() == pmem_exp::cend(c));

		UT_ASSERT(c.rbegin() == pmem_exp::rbegin(c));
		UT_ASSERT(c.crbegin() == pmem_exp::crbegin(c));
		UT_ASSERT(c.rend() == pmem_exp::rend(c));
		UT_ASSERT(c.crend() == pmem_exp::crend(c));

		UT_ASSERT(pmem_exp::begin(c) != pmem_exp::end(c));
		UT_ASSERT(pmem_exp::rbegin(c) != pmem_exp::rend(c));
		UT_ASSERT(pmem_exp::cbegin(c) != pmem_exp::cend(c));
		UT_ASSERT(pmem_exp::crbegin(c) != pmem_exp::crend(c));

		UT_ASSERT(*c.begin() == 0);
		UT_ASSERT(*c.rbegin() == 4);

		UT_ASSERT(*pmem_exp::begin(c) == 0);
		UT_ASSERT(*pmem_exp::cbegin(c) == 0);
		UT_ASSERT(*pmem_exp::rbegin(c) == 4);
		UT_ASSERT(*pmem_exp::crbegin(c) == 4);
	}

	return 0;
}
