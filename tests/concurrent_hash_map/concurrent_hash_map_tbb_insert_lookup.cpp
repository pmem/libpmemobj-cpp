// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * concurrent_hash_map_tbb_insert_lookup.cpp -- pmem::obj::concurrent_hash_map
 *                                              test with Intel TBB RW mutex
 *
 */

#define LIBPMEMOBJ_CPP_USE_TBB_RW_MUTEX 1
#include "concurrent_hash_map_insert_lookup.cpp"
