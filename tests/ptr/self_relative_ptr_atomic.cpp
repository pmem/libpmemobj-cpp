// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "self_relative_ptr_atomic.hpp"

int
main(int argc, char *argv[])
{
	return run_test([&] { test<false>(argc, argv); });
}
