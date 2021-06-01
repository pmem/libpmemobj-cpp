// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

#include <algorithm>
#include <string>

#include <libpmemobj++/experimental/mpsc_queue.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "layout"

struct root {
	pmem::obj::persistent_ptr<char[]> log;
};

/* Test to consume from empty queue */
int
consume_empty(pmem::obj::pool<root> pop)
{

	auto proot = pop.root();

	auto queue = pmem::obj::experimental::mpsc_queue(proot->log, 10000, 1);

	auto worker = queue.register_worker();
	bool consumed = queue.try_consume(
		[&](pmem::obj::experimental::mpsc_queue::read_accessor rd_acc) {
			ASSERT_UNREACHABLE;
		});
	UT_ASSERTeq(consumed, false);

	return 0;
}

/* Test if user may continue to consume, when all data is already consumed */
int
consume_empty_after_insertion(pmem::obj::pool<root> pop)
{
	size_t queue_size = 1000;
	auto proot = pop.root();
	auto queue =
		pmem::obj::experimental::mpsc_queue(proot->log, queue_size, 1);

	std::vector<std::string> values = {"xxx", "aaaaaaa", "bbbbb"};

	auto worker = queue.register_worker();
	/* Insert some data */
	for (const auto &e : values) {
		worker.try_produce(
			e.size(), [&](pmem::obj::slice<char *> range) {
				std::copy_n(e.begin(), e.size(), range.begin());
			});
	}
	/* Consume all of it */
	queue.try_consume(
		[&](pmem::obj::experimental::mpsc_queue::read_accessor rd_acc) {
			size_t i = 0;
			for (const auto &str : rd_acc) {
				(void)str;
				i++;
			}
			UT_ASSERTeq(i, values.size());
		});

	/* Try to consume empty queue */
	for (int i = 0; i < 10; i++) {
		bool consumed = queue.try_consume(
			[&](pmem::obj::experimental::mpsc_queue::read_accessor
				    rd_acc1) { ASSERT_UNREACHABLE; });
		UT_ASSERTeq(consumed, false);
	}

	return 0;
}

/* Basic try_produce-consume-recovery scenario */
int
basic_test(pmem::obj::pool<root> pop, bool create)
{

	auto proot = pop.root();

	auto queue = pmem::obj::experimental::mpsc_queue(proot->log, 10000, 1);
	auto worker = queue.register_worker();

	std::vector<std::string> values = {"xxx", "aaaaaaa", "bbbbb"};

	if (create) {

		for (const auto &e : values) {
			worker.try_produce(
				e.size(), [&](pmem::obj::slice<char *> range) {
					std::copy_n(e.begin(), e.size(),
						    range.begin());
				});
		}

		std::vector<std::string> values_on_pmem;
		queue.try_consume(
			[&](pmem::obj::experimental::mpsc_queue::read_accessor
				    rd_acc) {
				for (const auto &str : rd_acc) {
					values_on_pmem.emplace_back(str.data(),
								    str.size());
				}
			});
		UT_ASSERT(values_on_pmem == values);

		std::string tmp = "old";

		worker.try_produce(tmp.size(),
				   [&](pmem::obj::slice<char *> range) {
					   std::copy_n(tmp.begin(), tmp.size(),
						       range.begin());
				   });
	} else {
		std::vector<std::string> values_on_pmem;
		queue.recover(
			[&](pmem::obj::experimental::mpsc_queue::entry &entry) {
				values_on_pmem.emplace_back(entry.data,
							    entry.size);
			});
		UT_ASSERTeq(values_on_pmem.size(), 1);
		UT_ASSERTeq(values_on_pmem[0].size(), 3);
		UT_ASSERT(values_on_pmem[0] == std::string("old"));
		std::cout << values_on_pmem[0] << std::endl;
	}

	return 0;
}

int
main(int argc, char *argv[])
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
			pop.root()->log =
				pmem::obj::make_persistent<char[]>(10000);
		});
	} else {
		pop = pmem::obj::pool<root>::open(std::string(path), LAYOUT);
	}

	run_test([&] { basic_test(pop, create); });
	if (create) {
		run_test([&] {
			consume_empty(pop);
			consume_empty_after_insertion(pop);
		});
	}

	return 0;
}
