//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018-2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

int
main()
{
	/**
	 * Test pmem::obj::experimental::vector destructor
	 *
	 * Expects that destructor is not deleted and noexcept
	 */
	{
		using vector_type = pmem_exp::vector<int>;
		static_assert(std::is_nothrow_destructible<vector_type>::value,
			      "");
	}

	return 0;
}
