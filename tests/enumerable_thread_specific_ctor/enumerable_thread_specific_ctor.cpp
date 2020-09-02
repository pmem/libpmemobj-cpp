// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/make_persistent.hpp>

#include <set>

namespace nvobj = pmem::obj;

using test_t = size_t;
using container_type = pmem::detail::enumerable_thread_specific<test_t>;

// Adding more concurrency will increase DRD test time
const size_t concurrency = 16;

struct root {
	nvobj::persistent_ptr<container_type> pptr1;
	nvobj::persistent_ptr<container_type> pptr2;
	nvobj::persistent_ptr<container_type> pptr3;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "TLSTest: enumerable_thread_specific_ctor",
		PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr1 = nvobj::make_persistent<container_type>();
		});

		parallel_exec(concurrency, [&](size_t thread_index) {
			r->pptr1->local() = thread_index;
			pop.persist(&r->pptr1->local(),
				    sizeof(r->pptr1->local()));
		});

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_type>(r->pptr1);
			nvobj::delete_persistent<container_type>(r->pptr2);
			nvobj::delete_persistent<container_type>(r->pptr3);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
