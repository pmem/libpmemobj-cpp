/*
 * Copyright 2018-2019, Intel Corporation
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

#include "unittest.hpp"

struct root {
};

using Object = int[10240];

void
run_ctl_pool_prefault(pmem::obj::pool<struct root> &pop) try {
	pop.ctl_set<int>(ut_toUTF8(L"prefault.at_open"), 1);

	auto prefault_at_open =
		pop.ctl_get<int>(ut_toUTF8(L"prefault.at_open"));
	UT_ASSERTeq(prefault_at_open, 1);

	pop.ctl_set<int>(ut_toUTF8(L"prefault.at_open"), 0);

	prefault_at_open = pop.ctl_get<int>(ut_toUTF8(L"prefault.at_open"));
	UT_ASSERTeq(prefault_at_open, 0);
} catch (...) {
	UT_ASSERT(0);
}

void
run_ctl_pool_extend(pmem::obj::pool<struct root> &pop) try {
	/* disable auto-extend */
	pop.ctl_set<uint64_t>(ut_toUTF8(L"heap.size.granularity"), 0);

	pmem::obj::persistent_ptr<Object> ptr;
	try {
		/* allocate until OOM */
		while (true) {
			pmem::obj::make_persistent_atomic<Object>(pop, ptr);
		}
	} catch (...) {
	}

	pop.ctl_exec<uint64_t>(ut_toUTF8(L"heap.size.extend"), (1 << 20) * 10);

	/* next allocation should succeed */
	try {
		pmem::obj::make_persistent_atomic<Object>(pop, ptr);
	} catch (...) {
		UT_ASSERT(0);
	}
} catch (...) {
	UT_ASSERT(0);
}

void
run_ctl_global() try {
	pmem::obj::ctl_set<int>(ut_toUTF8(L"prefault.at_create"), 1);

	auto prefault_at_create =
		pmem::obj::ctl_get<int>(ut_toUTF8(L"prefault.at_create"));
	UT_ASSERTeq(prefault_at_create, 1);

	pmem::obj::ctl_set<int>(ut_toUTF8(L"prefault.at_create"), 0);

	prefault_at_create =
		pmem::obj::ctl_get<int>(ut_toUTF8(L"prefault.at_create"));
	UT_ASSERTeq(prefault_at_create, 0);
} catch (...) {
	UT_ASSERT(0);
}

void
run_ctl_exception()
{
	try {
		/* run query with non-existing entry point */
		pmem::obj::ctl_set<int>(
			ut_toUTF8(L"prefault.non_existing_entry_point"), 1);
		UT_ASSERT(0);
	} catch (pmem::ctl_error &e) {
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		/* run query with non-existing entry point */
		pmem::obj::ctl_get<int>(
			ut_toUTF8(L"prefault.non_existing_entry_point"));
		UT_ASSERT(0);
	} catch (pmem::ctl_error &e) {
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		/* run query with non-existing entry point */
		pmem::obj::ctl_exec<int>(
			ut_toUTF8(L"prefault.non_existing_entry_point"), 1);
		UT_ASSERT(0);
	} catch (pmem::ctl_error &e) {
	} catch (...) {
		UT_ASSERT(0);
	}
}

int
wmain(int argc, wchar_t *argv[])
{
	START();

	if (argc < 2) {
		UT_FATAL("usage: %s file-name", ut_toUTF8(argv[0]));
		return 1;
	}

	auto path = argv[1];
	auto pop = pmem::obj::pool<root>::create(path, L"ctl_test", 0,
						 S_IWUSR | S_IRUSR);

	run_ctl_pool_prefault(pop);
	run_ctl_pool_extend(pop);
	run_ctl_global();
	run_ctl_exception();

	pop.close();

	return 0;
}
