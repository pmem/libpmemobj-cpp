// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, 4Paradigm Inc. */

#include "pa_self_relative_ptr_atomic.hpp"

int
main(int argc, char *argv[])
{
	return run_test([&] { test<false>(argc, argv); });
}
