// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <chrono>

template <typename TimeUnit, typename F>
static typename TimeUnit::rep
measure(F &&func)
{
	auto start = std::chrono::steady_clock::now();

	func();

	auto duration = std::chrono::duration_cast<TimeUnit>(
		std::chrono::steady_clock::now() - start);
	return duration.count();
}
