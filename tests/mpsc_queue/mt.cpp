// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"
#include <cstring>
#include <iostream>

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#include <algorithm>
#include <libpmemobj++/container/mpsc_queue.hpp>
#include <string>

#define LAYOUT "multithreaded_mpsc_queue_test"

struct root {
	pmem::obj::persistent_ptr<char[]> log;
};

int
mt_test(int argc, char *argv[])
{
	if (argc != 3)
		UT_FATAL("usage: %s file-name create", argv[0]);

	const char *path = argv[1];
	bool create = std::string(argv[2]) == "1";

	size_t concurrency = 16;

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
		/* If running this test second time, on already exiting pool,
		 * the queue should be empty */
		pop = pmem::obj::pool<root>::open(std::string(path), LAYOUT);
	}

	auto proot = pop.root();

	auto queue = pmem::obj::experimental::mpsc_queue(proot->log, 100000,
							 concurrency);

	std::vector<std::string> values = {"xxx", "aaaaaaa", "bbbbb", "cccc"};

	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	std::atomic_bool execution_end(false);
	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			auto worker = queue.register_worker();
			int x = 0;
			for (auto &e : values) {
				bool insert_succeed = false;
				while (!insert_succeed) {
					insert_succeed = worker.produce(
						e.size(), [&](auto range) {
							x++;
							std::copy_n(
								e.begin(),
								e.size(),
								range.begin());
						});
				};
			}
			std::cout << x << std::endl;
		});
	};

	std::vector<std::string> values_on_pmem;
	std::thread consumer([&]() {
		/* Read data while writting */
		while (!execution_end.load()) {
			queue.consume([&](auto rd_acc) {
				for (auto str : rd_acc) {
					std::cout << "consume: " << str
						  << std::endl;
					values_on_pmem.emplace_back(str.data(),
								    str.size());
				}
			});
		}
	});

	for (auto &t : threads) {
		t.join();
	}
	execution_end.store(true);
	consumer.join();
	queue.consume([&](auto rd_acc1) {
		for (auto str : rd_acc1) {
			std::cout << str.size() << " ";
			values_on_pmem.emplace_back(str.data(), str.size());
		}
	});

	for (auto &v : values) {
		auto count = std::count(values_on_pmem.begin(),
					values_on_pmem.end(), v);
		UT_ASSERTeq(count, concurrency);
	}
	UT_ASSERT(false);
	return 0;
}

int
main(int argc, char *argv[])
{

	return run_test([&] { mt_test(argc, argv); });
}
