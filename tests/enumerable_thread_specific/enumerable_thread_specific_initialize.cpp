// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/make_persistent.hpp>

#include <set>

namespace nvobj = pmem::obj;

using test_t = nvobj::p<size_t>;
using container_type = pmem::detail::enumerable_thread_specific<test_t>;

struct root {
	nvobj::persistent_ptr<container_type> pptr;
};

void
create_and_fill(nvobj::pool<struct root> &pop, size_t concurrency)
{
	auto &tls = pop.root()->pptr;

	UT_ASSERT(tls == nullptr);

	nvobj::transaction::run(
		pop, [&] { tls = nvobj::make_persistent<container_type>(); });
	parallel_exec_with_sync(concurrency, [&](size_t thread_index) {
		tls->local() = thread_index;
		pop.persist(tls->local());
	});
	UT_ASSERT(tls->size() <= concurrency);
}

void
check_and_delete(nvobj::pool<struct root> &pop, size_t concurrency)
{
	auto &tls = pop.root()->pptr;

	std::set<size_t> checker;
	tls->initialize([&checker](test_t &e) {
		UT_ASSERT(checker.emplace(e).second);
	});
	UT_ASSERT(checker.size() <= concurrency);
	UT_ASSERT(tls->empty());

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_type>(tls);
		tls = nullptr;
	});
}

void
check_with_tx_and_delete(nvobj::pool<struct root> &pop, size_t concurrency)
{
	auto &tls = pop.root()->pptr;

	std::set<size_t> checker;
	nvobj::transaction::run(pop, [&] {
		tls->initialize([&checker](test_t &e) {
			UT_ASSERT(checker.emplace(e).second);
		});
	});

	UT_ASSERT(checker.size() <= concurrency);
	UT_ASSERT(tls->empty());

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_type>(tls);
		tls = nullptr;
	});
}

void
check_with_tx_abort_and_delete(nvobj::pool<struct root> &pop,
			       size_t concurrency)
{
	auto &tls = pop.root()->pptr;

	std::set<size_t> checker;

	try {
		nvobj::transaction::run(pop, [&] {
			tls->initialize([&checker](test_t &e) {
				UT_ASSERT(checker.emplace(e).second);
			});

			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(checker.size() <= concurrency);
	UT_ASSERT(!tls->empty());
	UT_ASSERT(tls->size() <= concurrency);

	for (auto &e : *tls)
		e = 0;

	parallel_exec_with_sync(concurrency, [&](size_t thread_index) {
		tls->local()++;
		pop.persist(&tls->local(), sizeof(tls->local()));
	});

	for (auto &e : *tls) {
		UT_ASSERTeq(e, 1);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_type>(tls);
		tls = nullptr;
	});
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto layout = "TLSTest: enumerable_thread_specific_initialize";
	auto pop = nvobj::pool<root>::create(path, layout, PMEMOBJ_MIN_POOL,
					     S_IWUSR | S_IRUSR);
	// Adding more concurrency will increase DRD test time
	size_t concurrency = 16;

	{
		create_and_fill(pop, concurrency);

		pop.close();
		pop = nvobj::pool<root>::open(path, layout);

		check_and_delete(pop, concurrency);
	}
	{
		create_and_fill(pop, concurrency);

		pop.close();
		pop = nvobj::pool<root>::open(path, layout);

		check_with_tx_and_delete(pop, concurrency);
	}
	{
		create_and_fill(pop, concurrency);

		pop.close();
		pop = nvobj::pool<root>::open(path, layout);

		check_with_tx_abort_and_delete(pop, concurrency);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
