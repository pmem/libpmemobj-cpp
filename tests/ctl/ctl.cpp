/*
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <libpmemobj.h>

#include "unittest.hpp"

struct root {
};

using Object = int[10240];

void
run_ctl_pool_alloc_class(pmem::obj::pool<struct root> &pop) try {
	pop.ctl_set<int>("prefault.at_open", 1);

	auto prefault_at_open = pop.ctl_get<int>("prefault.at_open");
	UT_ASSERTeq(prefault_at_open, 1);

	pop.ctl_set<int>("prefault.at_open", 0);

	prefault_at_open = pop.ctl_get<int>("prefault.at_open");
	UT_ASSERTeq(prefault_at_open, 0);
} catch (...) {
	UT_ASSERT(0);
}

void
run_ctl_pool_extend(pmem::obj::pool<struct root> &pop) try {
	/* disable auto-extend */
	pop.ctl_set<uint64_t>("heap.size.granularity", 0);

	try {
		/* allocate until OOM */
		while (true) {
			pmem::obj::persistent_ptr<Object> ptr;
			pmem::obj::make_persistent_atomic<Object>(pop, ptr);
		}
	} catch (...) {
	}

	pop.ctl_exec<uint64_t>("heap.size.extend", (1 << 20) * 10);

	/* next allocation should succeed */
	try {
		pmem::obj::persistent_ptr<Object> ptr;
		pmem::obj::make_persistent_atomic<Object>(pop, ptr);
	} catch (...) {
		UT_ASSERT(0);
	}
} catch (...) {
	UT_ASSERT(0);
}

void
run_ctl_global() try {
	pmem::obj::ctl_set<int>("prefault.at_create", 1);

	auto prefault_at_create = pmem::obj::ctl_get<int>("prefault.at_create");
	UT_ASSERTeq(prefault_at_create, 1);

	pmem::obj::ctl_set<int>("prefault.at_create", 0);

	prefault_at_create = pmem::obj::ctl_get<int>("prefault.at_create");
	UT_ASSERTeq(prefault_at_create, 0);
} catch (...) {
	UT_ASSERT(0);
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[1]);
		return 1;
	}

	auto path = argv[1];
	auto pop = pmem::obj::pool<root>::create(path, "ctl_test", 0,
						 S_IWUSR | S_IRUSR);

	run_ctl_pool_alloc_class(pop);
	run_ctl_pool_extend(pop);
	run_ctl_global();

	pop.close();

	return 0;
}
