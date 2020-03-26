// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using test_t = int;

using container_type = pmem::detail::enumerable_thread_specific<test_t>;

struct root {
	nvobj::persistent_ptr<container_type> pptr;
};

void
test(nvobj::pool<struct root> &pop, size_t batch_size)
{
	const size_t num_batches = 3;

	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());

	for (size_t i = 0; i < num_batches; i++)
		parallel_exec(batch_size, [&](size_t thread_index) {
			tls->local();
			pop.persist(&tls->local(), sizeof(tls->local()));
		});

	/* There was at most batch_size threads at any given time. */
	UT_ASSERT(tls->size() <= batch_size);

	tls->clear();
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());
}

void
test_with_spin(nvobj::pool<struct root> &pop, size_t batch_size)
{
	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());

	parallel_exec_with_sync(batch_size, [&](size_t thread_index) {
		tls->local() = thread_index;
		pop.persist(&tls->local(), sizeof(tls->local()));
	});

	/*
	 * tls->size() will be equal to max number of threads that have used
	 * tls at any given time. This test assumes that batch_size is >=
	 * than any previously used number of threads
	 */
	UT_ASSERTeq(tls->size(), batch_size);

	tls->clear();
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());
}

void
test_clear_abort(nvobj::pool<struct root> &pop, size_t batch_size)
{
	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());

	parallel_exec_with_sync(batch_size, [&](size_t thread_index) {
		tls->local() = 2;
		pop.persist(&tls->local(), sizeof(tls->local()));
	});

	/*
	 * tls->size() will be equal to max number of threads that have used
	 * tls at any given time. This test assumes that batch_size is >=
	 * than any previously used number of threads
	 */
	UT_ASSERTeq(tls->size(), batch_size);

	try {
		nvobj::transaction::run(pop, [&] {
			tls->clear();

			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(tls->size(), batch_size);

	for (auto &e : *tls)
		UT_ASSERTeq(e, 2);
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "TLSTest: enumerable_thread_specific_size",
		PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<container_type>();
		});

		test(pop, 8);
		test(pop, 10);

		test_with_spin(pop, 12);
		test_with_spin(pop, 16);

		test_clear_abort(pop, 16);

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
