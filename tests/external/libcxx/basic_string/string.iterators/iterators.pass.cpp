//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

int
main()
{
	{ // N3644 testing
		using C = pmem::obj::string;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);
		UT_ASSERT(ii1 == cii);
		UT_ASSERT(!(ii1 != ii2));
		UT_ASSERT(!(ii1 != cii));
	}

	{ // N3644 testing
		using C = pmem::obj::wstring;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);
		UT_ASSERT(ii1 == cii);
		UT_ASSERT(!(ii1 != ii2));
		UT_ASSERT(!(ii1 != cii));
	}

	{ // N3644 testing
		using C = pmem::obj::u16string;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);
		UT_ASSERT(ii1 == cii);
		UT_ASSERT(!(ii1 != ii2));
		UT_ASSERT(!(ii1 != cii));
	}

	{ // N3644 testing
		using C = pmem::obj::u32string;
		C::iterator ii1{}, ii2{};
		C::iterator ii4 = ii1;
		C::const_iterator cii{};
		UT_ASSERT(ii1 == ii2);
		UT_ASSERT(ii1 == ii4);
		UT_ASSERT(ii1 == cii);
		UT_ASSERT(!(ii1 != ii2));
		UT_ASSERT(!(ii1 != cii));
	}

	return 0;
}
