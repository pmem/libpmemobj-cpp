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

#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

int
main(int argc, char *argv[])
{
	START();

	{ // N3644 testing
		typedef pmem_exp::string C;
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
		typedef pmem_exp::wstring C;
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
		typedef pmem_exp::u16string C;
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
		typedef pmem_exp::u32string C;
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
