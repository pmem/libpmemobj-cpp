// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * consume_interrupt.cpp -- Test for mpsc_queue which interrupts
 * try_consume_batch and check if the data is accessible afterwards.
 */

#include "unittest.hpp"

#include <algorithm>
#include <string>

#include <libpmemobj++/experimental/mpsc_queue.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "layout"

using queue_type = pmem::obj::experimental::mpsc_queue;

static constexpr size_t QUEUE_SIZE = 10000;

struct root {
	pmem::obj::persistent_ptr<queue_type::pmem_log_type> log;
};

/* Basic try_produce-consume-recovery scenario */
static void
consume_interrupt(pmem::obj::pool<root> pop, bool create)
{
	auto proot = pop.root();

	auto queue = queue_type(*proot->log, 1);

	auto worker = queue.register_worker();

	std::vector<std::string> values = {"xxx", "aaaaaaa", "bbbbb",
					   std::string(120, 'a')};

	if (create) {
		auto ret = queue.try_consume_batch(
			[&](queue_type::batch_type acc) {
				ASSERT_UNREACHABLE;
			});
		UT_ASSERT(!ret);

		/* XXX: this is to make sure that try_consume_batch later in the
		 * test returns all elements within a single callback call. */
		ret = worker.try_produce(values[0]);
		UT_ASSERT(ret);
		ret = queue.try_consume_batch(
			[&](queue_type::batch_type rd_acc) {
				std::vector<std::string> v;
				for (const auto &str : rd_acc)
					v.emplace_back(str.data(), str.size());

				UT_ASSERTeq(v.size(), 1);
				UT_ASSERT(v[0] == values[0]);
			});
		UT_ASSERT(ret);

		/* Insert the data */
		for (const auto &e : values) {
			ret = worker.try_produce(e);
			UT_ASSERT(ret);
		}

		/* Consume all the data */
		std::vector<std::string> values_on_pmem;
		const int retries = 3;

		for (int i = 0; i < retries; i++) {
			try {
				ret = queue.try_consume_batch(
					[&](queue_type::batch_type rd_acc) {
						for (const auto &str : rd_acc) {
							values_on_pmem.emplace_back(
								str.data(),
								str.size());
						}

						throw std::runtime_error("");
					});
				ASSERT_UNREACHABLE;
			} catch (std::runtime_error &) {
			} catch (...) {
				ASSERT_UNREACHABLE;
			}
		}

		UT_ASSERTeq(values_on_pmem.size(), values.size() * retries);
		for (auto &str : values) {
			auto count = std::count(values_on_pmem.begin(),
						values_on_pmem.end(), str);
			UT_ASSERTeq(count, retries);
		}
	} else {
		std::vector<std::string> values_on_pmem;
		/* Recover the data in second run of application */
		auto ret = queue.try_consume_batch(
			[&](queue_type::batch_type acc) {
				for (const auto &entry : acc)
					values_on_pmem.emplace_back(
						entry.data(), entry.size());
			});
		UT_ASSERT(ret);
		UT_ASSERTeq(values_on_pmem.size(), values.size());
		for (auto &str : values) {
			auto count = std::count(values_on_pmem.begin(),
						values_on_pmem.end(), str);
			UT_ASSERTeq(count, 1);
		}
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 3)
		UT_FATAL("usage: %s file-name create", argv[0]);

	const char *path = argv[1];
	bool create = std::string(argv[2]) == "1";

	pmem::obj::pool<struct root> pop;

	if (create) {
		pop = pmem::obj::pool<root>::create(std::string(path), LAYOUT,
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);

		pmem::obj::transaction::run(pop, [&] {
			pop.root()->log = pmem::obj::make_persistent<
				queue_type::pmem_log_type>(QUEUE_SIZE);
		});
	} else {
		pop = pmem::obj::pool<root>::open(std::string(path), LAYOUT);
	}

	consume_interrupt(pop, create);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
