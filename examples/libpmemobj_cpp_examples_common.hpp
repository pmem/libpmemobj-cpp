// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_COMMON_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_COMMON_HPP

#include <cstdint>

#ifndef _WIN32

#include <unistd.h>

#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

/*
 * file_exists -- checks if file exists
 */
static inline int
file_exists(char const *file)
{
	return access(file, F_OK);
}

/*
 * find_last_set_64 -- returns last set bit position or -1 if set bit not found
 */
static inline int
find_last_set_64(uint64_t val)
{
	return 64 - __builtin_clzll(val) - 1;
}

#else

#include <corecrt_io.h>
#include <process.h>
#include <windows.h>

#define CREATE_MODE_RW (S_IWRITE | S_IREAD)

/*
 * file_exists -- checks if file exists
 */
static inline int
file_exists(char const *file)
{
	return _access(file, 0);
}

/*
 * find_last_set_64 -- returns last set bit position or -1 if set bit not found
 */
static inline int
find_last_set_64(uint64_t val)
{
	DWORD lz = 0;

	if (BitScanReverse64(&lz, val))
		return (int)lz;
	else
		return -1;
}

#endif

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_COMMON_HPP */
