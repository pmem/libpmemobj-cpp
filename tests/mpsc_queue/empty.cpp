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

/* Test to consume from empty queue */
static void
consume_empty(pmem::obj::pool<root> pop)
{
	auto proot = pop.root();
	auto queue = queue_type(*proot->log, 1);
	auto worker = queue.register_worker();

	bool consumed = queue.try_consume_batch(
		[&](queue_type::batch_type rd_acc) { ASSERT_UNREACHABLE; });
	UT_ASSERTeq(consumed, false);
}

/* Test if user may continue to consume, when all data is already consumed */
static void
consume_empty_after_insertion(pmem::obj::pool<root> pop)
{
	auto proot = pop.root();
	auto queue = queue_type(*proot->log, 1);

	bool consumed = queue.try_consume_batch(
		[&](queue_type::batch_type rd_acc) { ASSERT_UNREACHABLE; });
	UT_ASSERTeq(consumed, false);

	std::vector<std::string> values = {
		"xxx", "aaaaaaa", "bbbbb",
		std::string(QUEUE_SIZE / 2 - 1, 'a')};

	auto worker = queue.register_worker();
	/* Insert some data */
	for (const auto &e : values) {
		auto ret = worker.try_produce(e);
		UT_ASSERT(ret);
	}
	/* Consume all of it */
	size_t i = 0;
	auto ret = queue.try_consume_batch([&](queue_type::batch_type rd_acc) {
		for (const auto &str : rd_acc) {
			(void)str;
			i++;
		}
	});
	UT_ASSERT(ret);
	UT_ASSERTeq(i, values.size());

	/* Try to consume empty queue */
	for (int i = 0; i < 10; i++) {
		bool consumed = queue.try_consume_batch(
			[&](queue_type::batch_type rd_acc1) {
				ASSERT_UNREACHABLE;
			});
		UT_ASSERTeq(consumed, false);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<struct root> pop;

	pop = pmem::obj::pool<root>::create(
		std::string(path), LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	pmem::obj::transaction::run(pop, [&] {
		pop.root()->log =
			pmem::obj::make_persistent<queue_type::pmem_log_type>(
				QUEUE_SIZE);
	});

	consume_empty(pop);
	consume_empty_after_insertion(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
