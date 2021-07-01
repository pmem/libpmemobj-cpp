// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/**
 * mpsc_queue.cpp -- example which shows how to use
 * pmem::obj::experimental::mpsc_queue
 */

#include <libpmemobj++/experimental/mpsc_queue.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#include <iostream>
#include <string>

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
}

//! [mpsc_queue_single_threaded_example]

struct root {
	pmem::obj::persistent_ptr<
		pmem::obj::experimental::mpsc_queue::pmem_log_type>
		log;
};

void
single_threaded(pmem::obj::pool<root> pop)
{

	std::vector<std::string> values_to_produce = {"xxx", "aaaaaaa", "bbbbb",
						      "cccc", "ddddddddddd"};
	pmem::obj::persistent_ptr<root> proot = pop.root();

	/* Create mpsc_queue runtime, which uses pmem_log_type object to store
	 * data */
	auto queue = pmem::obj::experimental::mpsc_queue(*proot->log, 1);

	/* Consume data, which was left in the queue from the previous run of
	 * application */
	queue.try_consume_batch(
		[&](pmem::obj::experimental::mpsc_queue::batch_type rd_acc) {
			for (pmem::obj::string_view str : rd_acc) {
				std::cout << str.data() << std::endl;
			}
		});
	/* Produce and consume data */
	pmem::obj::experimental::mpsc_queue::worker worker =
		queue.register_worker();

	for (size_t i = 0; i < values_to_produce.size(); i++) {
		std::string v = values_to_produce[i];
		worker.try_produce(
			v.size(), [&](pmem::obj::slice<char *> range) {
				std::copy_n(v.begin(), v.size(), range.begin());
			});
		/* consume produced data */
		queue.try_consume_batch(
			[&](pmem::obj::experimental::mpsc_queue::batch_type
				    rd_acc) {
				for (pmem::obj::string_view str : rd_acc) {
					std::cout << std::string(str.data(),
								 str.size())
						  << std::endl;
				}
			});
	}
	/* Porduce data to be for next run */
	worker.try_produce("Left for next run");
}

//! [mpsc_queue_single_threaded_example]

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		show_usage(argv);
		return 1;
	}

	const char *path = argv[1];

	static constexpr size_t QUEUE_SIZE = 1000;

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::open(path, "mpsc_queue");
		if (pop.root()->log == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				pop.root()->log = pmem::obj::make_persistent<
					pmem::obj::experimental::mpsc_queue::
						pmem_log_type>(QUEUE_SIZE);
			});
		}
		single_threaded(pop);

	} catch (pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr
			<< "To create pool run: pmempool create obj --layout=mpsc_queue -s 100M path_to_pool"
			<< std::endl;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
