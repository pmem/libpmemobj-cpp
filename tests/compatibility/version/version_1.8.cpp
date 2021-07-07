// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include <libpmemobj++/version.hpp>

int
main()
{
	static_assert(LIBPMEMOBJ_CPP_VERSION_MAJOR == 1, "Wrong major version");
	static_assert(LIBPMEMOBJ_CPP_VERSION_MINOR == 8, "Wrong minor version");
}
