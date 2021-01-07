// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2017-2020, Intel Corporation */

#include <cstdlib>
#include <dlfcn.h>
#include <errno.h>
#include <libpmemobj/tx_base.h>

#include "mock_tx_alloc.h"

size_t test_alloc_counter;

extern "C" PMEMoid pmemobj_tx_alloc(size_t size, uint64_t type_num);

PMEMoid
pmemobj_tx_alloc(size_t size, uint64_t type_num)
{
	static auto real = (decltype(pmemobj_tx_alloc) *)dlsym(
		RTLD_NEXT, "pmemobj_tx_alloc");

	if (real == nullptr)
		abort();

	test_alloc_counter++;

	return real(size, type_num);
}

PMEMoid
pmemobj_tx_xalloc(size_t size, uint64_t type_num, uint64_t flags)
{
	static auto real = (decltype(pmemobj_tx_xalloc) *)dlsym(
		RTLD_NEXT, "pmemobj_tx_xalloc");

	if (real == nullptr)
		abort();

	test_alloc_counter++;

	return real(size, type_num, flags);
}
