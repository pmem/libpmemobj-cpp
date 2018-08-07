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

/*
 * action.cpp -- cpp action class test
 */
#include "unittest.hpp"

#include <libpmemobj++/detail/action.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{
struct test_val {
	uint64_t val;
};

struct root {
	struct {
		nvobj::persistent_ptr<test_val> ptr;
		uint64_t val;
	} test_reserve;

	struct {
		nvobj::persistent_ptr<test_val> ptr;
		uint64_t val;
	} test_publish;
};

static void
test_action_reserve(nvobj::pool<root> &pop, const char *path)
{
	try {
		auto rootp = pop.get_root();

		pmem::detail::action act(pop);
		rootp->test_reserve.ptr = act.reserve<test_val>();

		act.set_value(&rootp->test_reserve.val, 1);
		rootp.persist();

		pop.close();

		pop = nvobj::pool<root>::open(path, LAYOUT);
		rootp = pop.get_root();

		struct test_val *tmp = rootp->test_reserve.ptr.get();
		tmp->val = 1; /* should trigger memcheck error */
		UT_ASSERTeq(rootp->test_reserve.val, 0);
	} catch (...) {
		UT_ASSERT(0);
	}
}

static void
test_action_publish(nvobj::pool<root> &pop, const char *path)
{
	try {
		auto rootp = pop.get_root();

		pmem::detail::action act(pop);
		rootp->test_publish.ptr = act.reserve<test_val>();

		act.set_value(&rootp->test_publish.val, 1);
		act.publish();

		rootp.persist();

		pop.close();
		UT_ASSERTeq(pop.check(path, LAYOUT), 1);
		pop = nvobj::pool<root>::open(path, LAYOUT);
		rootp = pop.get_root();

		struct test_val *tmp = rootp->test_publish.ptr.get();
		tmp->val = 1; /* should NOT trigger memcheck error */
		UT_ASSERTeq(rootp->test_publish.val, 1);
	} catch (...) {
		UT_ASSERT(0);
	}
}

static void
test_action_defer_free(nvobj::pool<root> &pop)
{
	try {
		nvobj::persistent_ptr<test_val> ptr;
		nvobj::make_persistent_atomic<test_val>(pop, ptr);

		pmem::detail::action act(pop);
		act.defer_free<test_val>(ptr);
		act.publish();

		struct test_val *tmp = ptr.get();
		tmp->val = 1; /* should trigger memcheck error */

		nvobj::make_persistent_atomic<test_val>(pop, ptr);

		act.defer_free<test_val>(ptr);
		act.cancel();

		tmp = ptr.get();
		tmp->val = 1; /* should NOT trigger memcheck error */

		nvobj::delete_persistent_atomic<test_val>(ptr);
	} catch (...) {
		UT_ASSERT(0);
	}
}
}

int
main(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;
	try {
		pop = nvobj::pool<root>::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	test_action_reserve(pop, path);
	test_action_publish(pop, path);
	test_action_defer_free(pop);

	pop.close();
}
