// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * wrap_pmemobj_defrag.h -- mock function for pmemobj_defrag()
 * used in the concurrent_hash_map tests which are run under Valgrind
 * in order to test only the concurrent_hash_map part of implementation
 * of the defragment() method and not pmemobj_defrag() itself.
 * These tests would last too long without this mock function.
 */

#include <libpmemobj/atomic_base.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * __wrap_pmemobj_defrag -- mock function for pmemobj_defrag()
 */
int
__wrap_pmemobj_defrag(PMEMobjpool *pop, PMEMoid **oidv, size_t oidcnt,
		      struct pobj_defrag_result *result)
{
	return 0;
}

#ifdef __cplusplus
}
#endif
