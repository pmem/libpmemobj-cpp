// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2018, Intel Corporation */

/**
 * @file
 * Commonly used conversions.
 */

#ifndef LIBPMEMOBJ_CPP_CONVERSIONS_HPP
#define LIBPMEMOBJ_CPP_CONVERSIONS_HPP

#include <chrono>
#include <ctime>

namespace pmem
{

namespace detail
{

/**
 * Convert std::chrono::time_point to posix timespec.
 *
 * @param[in] timepoint point in time to be converted.
 *
 * @return converted timespec structure.
 */
template <typename Clock, typename Duration = typename Clock::duration>
timespec
timepoint_to_timespec(const std::chrono::time_point<Clock, Duration> &timepoint)
{
	timespec ts;
	auto rel_duration = timepoint.time_since_epoch();
	const auto sec =
		std::chrono::duration_cast<std::chrono::seconds>(rel_duration);

	ts.tv_sec = sec.count();
	ts.tv_nsec = static_cast<long>(
		std::chrono::duration_cast<std::chrono::nanoseconds>(
			rel_duration - sec)
			.count());

	return ts;
}

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_CONVERSIONS_HPP */
