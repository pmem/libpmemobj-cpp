// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * recovery_order.pp -- Check if recovery returns element in correct order.
 */

#include "queue.hpp"

#include <algorithm>
#include <string>

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

static void
test_recovery(pmem::obj::pool<root> pop, bool create)
{
	auto proot = pop.root();

	auto queue = queue_type(*proot->log, 2);

	if (create) {
		bool consumed = queue.try_consume_batch(
			[&](queue_type::batch_type rd_acc) {
				ASSERT_UNREACHABLE;
			});
		UT_ASSERTeq(consumed, false);

		size_t capacity = get_queue_capacity(queue);
		UT_ASSERTne(capacity, 0);

		size_t cnt = 0;

		make_queue_with_first_half_empty(queue);
		auto worker = queue.register_worker();
		/* Produce elements with different sizes */
		while (worker.try_produce(std::to_string(cnt))) {
			cnt++;
		}
	} else {
		std::vector<std::string> values_on_pmem;
		/* Recover the data in second run of application */
		queue.try_consume_batch([&](queue_type::batch_type rd_acc) {
			for (auto entry : rd_acc) {
				auto element =
					std::string(entry.data(), entry.size());
				if (entry != "x")
					values_on_pmem.emplace_back(element);
			}
		});

		UT_ASSERTne(values_on_pmem.size(), 0);
		std::vector<std::string> sorted_values = values_on_pmem;
		std::sort(sorted_values.begin(), sorted_values.end(),
			  [](const std::string &a, const std::string &b) {
				  return std::stoi(a) < std::stoi(b);
			  });
		UT_ASSERT(values_on_pmem == sorted_values);
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

	test_recovery(pop, create);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
