// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/make_persistent.hpp>

#include <set>
#include <vector>

namespace nvobj = pmem::obj;

using test_t = std::size_t;

using container_type = pmem::detail::enumerable_thread_specific<test_t>;

struct root {
	nvobj::persistent_ptr<container_type> pptr;

	nvobj::persistent_ptr<container_type> m_pptr1;
	nvobj::persistent_ptr<container_type> m_pptr2;
};

void
test(nvobj::pool<struct root> &pop)
{
	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 16;

	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);

	{
		std::vector<size_t> checker(concurrency, 0);
		parallel_exec(concurrency, [&](size_t thread_index) {
			test_t &ref = tls->local();

			/*
			 * Another thread already wrote some data there
			 * (and exited).
			 */
			if (ref > 0)
				return;

			ref = thread_index;
			for (size_t i = 0; i < 100; ++i) {
				ref = tls->local();
				UT_ASSERTeq(ref, thread_index);

				checker[ref]++;
			}
		});

		UT_ASSERT(tls->size() <= concurrency);

		size_t n_zeros = 0;
		size_t n_100 = 0;
		for (auto &e : checker) {
			if (e == 0)
				n_zeros++;
			else if (e == 100)
				n_100++;
			else
				UT_ASSERTeq(e, 0);
		}

		/* At least one thread should have done its work */
		UT_ASSERT(n_100 > 0);
		UT_ASSERT(n_100 + n_zeros == concurrency);
	}

	std::thread t([&] {
		tls->local() = 99;
		pop.persist(&tls->local(), sizeof(tls->local()));

		UT_ASSERT(tls->size() <= concurrency + 1);
		UT_ASSERT(tls->local() == 99);
	});

	t.join();

	tls->clear();
}

void
test_with_spin(nvobj::pool<struct root> &pop, size_t concurrency)
{
	auto tls = pop.root()->pptr;

	UT_ASSERT(tls != nullptr);
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());

	parallel_exec_with_sync(concurrency, [&](size_t thread_index) {
		tls->local()++;
		pop.persist(&tls->local(), sizeof(tls->local()));
	});

	/*
	 * tls->size() will be equal to max number of threads that have used
	 * tls at any given time. This test assumes that concurrency is >=
	 * than any previously used number of threads
	 */
	UT_ASSERTeq(tls->size(), concurrency);

	for (auto &e : *tls) {
		UT_ASSERTeq(e, 1);
	}

	tls->clear();
	UT_ASSERT(tls->size() == 0);
	UT_ASSERT(tls->empty());
}

void
test_multiple_tls(nvobj::pool<struct root> &pop)
{
	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 16;

	auto tls1 = pop.root()->m_pptr1;
	auto tls2 = pop.root()->m_pptr2;

	parallel_exec_with_sync(concurrency, [&](size_t thread_index) {
		tls1->local() = thread_index;
		pop.persist(&tls1->local(), sizeof(tls1->local()));
	});

	parallel_exec_with_sync(concurrency, [&](size_t thread_index) {
		tls2->local() = thread_index;
		pop.persist(&tls2->local(), sizeof(tls2->local()));
	});

	UT_ASSERT(tls1->size() == concurrency);
	UT_ASSERT(tls2->size() == concurrency);

	{
		std::set<size_t> tids;
		for (auto &e : *tls1)
			tids.insert(e);

		for (size_t id = 0; id < concurrency; id++)
			UT_ASSERT(tids.count(id) == 1);
	}

	{
		std::set<size_t> tids;
		for (auto &e : *tls2)
			tids.insert(e);

		for (size_t id = 0; id < concurrency; id++)
			UT_ASSERT(tids.count(id) == 1);
	}

	tls1->clear();
	tls2->clear();

	UT_ASSERT(tls1->size() == 0);
	UT_ASSERT(tls2->size() == 0);

	parallel_exec_with_sync(concurrency, [&](size_t thread_index) {
		tls1->local() = thread_index;
		pop.persist(&tls1->local(), sizeof(tls1->local()));

		tls2->local() = thread_index;
		pop.persist(&tls2->local(), sizeof(tls2->local()));
	});

	UT_ASSERT(tls1->size() == concurrency);
	UT_ASSERT(tls2->size() == concurrency);

	{
		std::set<size_t> tids;
		for (auto &e : *tls1)
			tids.insert(e);

		for (size_t id = 0; id < concurrency; id++)
			UT_ASSERT(tids.count(id) == 1);
	}

	{
		std::set<size_t> tids;
		for (auto &e : *tls2)
			tids.insert(e);

		for (size_t id = 0; id < concurrency; id++)
			UT_ASSERT(tids.count(id) == 1);
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
		path, "TLSTest: enumerable_thread_specific_access",
		10 * PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr = nvobj::make_persistent<container_type>();
			r->m_pptr1 = nvobj::make_persistent<container_type>();
			r->m_pptr2 = nvobj::make_persistent<container_type>();
		});

		test(pop);
		test_multiple_tls(pop);
		test_with_spin(pop, 16);

		if (!On_valgrind) {
			/*
			 * larger that initial size of queue of thread ids,
			 * run this only when not on valgrind due to execution
			 * time.
			 */
			test_with_spin(pop, 2048);
		}

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_type>(r->pptr);
			nvobj::delete_persistent<container_type>(r->m_pptr1);
			nvobj::delete_persistent<container_type>(r->m_pptr2);
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
