// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * basic.pp -- Single threaded tests for
 * pmem::obj::experimental::mpsc_queue
 */

#include "unittest.hpp"

#include <algorithm>
#include <string>

#include <libpmemobj++/experimental/mpsc_queue.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "layout"

using queue_type = pmem::obj::experimental::mpsc_queue;

static constexpr size_t QUEUE_SIZE = 10000;

struct root {
	pmem::obj::persistent_ptr<queue_type::pmem_log_type> log;
};

/* Iterate over data in consume multiple times */
static void
consume_multipass(pmem::obj::pool<root> pop, size_t n_iters)
{
	auto proot = pop.root();
	auto queue = queue_type(*proot->log, 1);
	auto worker = queue.register_worker();

	std::vector<std::string> values = {"xxx", "aaaaaaa", "bbbbb",
					   std::string(120, 'a')};
	std::string store_to_next_run = "old";

	auto ret = queue.try_consume_batch(
		[&](queue_type::batch_type acc) { ASSERT_UNREACHABLE; });
	UT_ASSERT(!ret);

	/* Insert the data */
	for (const auto &e : values) {
		auto ret = worker.try_produce(e);
		UT_ASSERT(ret);
	}

	/* Consume all the data */
	std::vector<std::string> values_on_pmem;
	ret = queue.try_consume_batch([&](queue_type::batch_type rd_acc) {
		for (size_t i = 0; i < n_iters; i++) {
			for (const auto &str : rd_acc) {
				values_on_pmem.emplace_back(str.data(),
							    str.size());
			}
		}
	});
	UT_ASSERT(ret);
	UT_ASSERTeq(values_on_pmem.size(), values.size() * n_iters);

	for (size_t i = 0; i < values.size() * n_iters; i++) {
		UT_ASSERT(values_on_pmem[i] == values[i % values.size()]);
	}

	ret = queue.try_consume_batch(
		[&](queue_type::batch_type acc) { ASSERT_UNREACHABLE; });
	UT_ASSERT(!ret);
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<struct root> pop = pmem::obj::pool<root>::create(
		std::string(path), LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	pmem::obj::transaction::run(pop, [&] {
		pop.root()->log =
			pmem::obj::make_persistent<queue_type::pmem_log_type>(
				QUEUE_SIZE);
	});

	consume_multipass(pop, 0);
	consume_multipass(pop, 2);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
