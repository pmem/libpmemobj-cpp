// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include <libpmemobj++/version.hpp>

int
main()
{
	static_assert(LIBPMEMOBJ_CPP_VERSION_MAJOR == 1, "");
	static_assert(LIBPMEMOBJ_CPP_VERSION_MINOR == 11, "");
}
