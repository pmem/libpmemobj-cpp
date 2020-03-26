// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using container_type = pmem::detail::enumerable_thread_specific<size_t>;

struct root {
	nvobj::persistent_ptr<container_type> pptr;
};

void
test(nvobj::pool<struct root> &pop)
{
	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 16;

	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);

	parallel_exec(concurrency, [&](size_t thread_index) {
		tls->local() = 99;
		pop.persist(&tls->local(), sizeof(tls->local()));
	});

	UT_ASSERT(tls->size() <= concurrency);

	container_type &ref = *tls;
	for (auto &e : ref) {
		UT_ASSERT(e == 99);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "TLSTest: enumerable_thread_specific_iterators",
		PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<container_type>();
		});

		test(pop);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_type>(r->pptr);
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
