// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"
#include <libpmemobj++/container/array.hpp>

/* Test which checks if array behave in the same way as any structure with
 * defaulted constructor - accordingly to used c++ standard */
struct struct_with_defaulted_constructor {
	struct_with_defaulted_constructor() = default;
};

int
main()
{
	bool array_is_aggregate =
		std::is_aggregate<pmem::obj::array<int, 1>>::value;
	bool cpp_standard_support =
		std::is_aggregate<struct_with_defaulted_constructor>::value;

	UT_ASSERTeq(array_is_aggregate, cpp_standard_support);
}
