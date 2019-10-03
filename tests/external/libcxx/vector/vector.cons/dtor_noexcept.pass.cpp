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

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>

int
main()
{
	/**
	 * Test pmem::obj::vector destructor
	 *
	 * Expects that destructor is not deleted and noexcept
	 */
	{
		using vector_type = container_t<int>;
		static_assert(std::is_nothrow_destructible<vector_type>::value,
			      "");
	}

	return 0;
}
