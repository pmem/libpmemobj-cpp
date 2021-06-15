// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * mt.pp -- Multithreaded tests for
 * pmem::obj::experimental::mpsc_queue
 */

#include "unittest.hpp"

#include <algorithm>
#include <atomic>
#include <string>

#include <libpmemobj++/experimental/mpsc_queue.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "multithreaded_mpsc_queue_test"

using queue_type = pmem::obj::experimental::mpsc_queue;

/* buffer_size have to be at least twice as big as biggest inserted
 * element */
static constexpr size_t QUEUE_SIZE = 2 * pmem::detail::CACHELINE_SIZE;

struct root {
	pmem::obj::persistent_ptr<queue_type::pmem_log_type> log;
};

/* basic multithreaded for produce-consume */
int
mt_test(pmem::obj::pool<root> pop, size_t concurrency)
{
	auto proot = pop.root();

	auto queue = queue_type(*proot->log, concurrency);

	std::vector<std::string> values = {"xxx", "aaaaaaa", "bbbbb", "cccc"};

	std::atomic<size_t> threads_counter(concurrency);

	std::vector<std::string> values_on_pmem;
	parallel_exec(concurrency + 1, [&](size_t thread_id) {
		if (thread_id == 0) {
			/* Read data while writting */
			while (threads_counter.load() > 0) {
				queue.try_consume([&](pmem::obj::experimental::
							      mpsc_queue::read_accessor
								      rd_acc) {
					for (auto str : rd_acc) {
						values_on_pmem.emplace_back(
							str.data(), str.size());
					}
				});
			}
			UT_ASSERTeq(values_on_pmem.empty(), false);
		} else {
			/* Concurrently add data to queue */
			auto worker = queue.register_worker();
			size_t x = 0;
			for (auto &e : values) {
				bool insert_succeed = false;
				while (!insert_succeed) {
					insert_succeed = worker.try_produce(
						e.size(),
						[&](pmem::obj::slice<char *>
							    range) {
							x++;
							std::copy_n(
								e.begin(),
								e.size(),
								range.begin());
						});
				};
			}
			UT_ASSERTeq(x, values.size());
			threads_counter--;
		}
	});

	/* Consume the rest of the data. Need to call try_consume twice, as some
	 * data may be at the end of buffer, and some may be at the beginning.
	 * Ringbuffer do not merge those two pats into one try_consume. If all
	 * data was consumed during first try_consume, second would fail.
	 */
	for (int i = 0; i < 2; i++) {
		queue.try_consume([&](queue_type::read_accessor rd_acc1) {
			for (auto str : rd_acc1) {
				values_on_pmem.emplace_back(str.data(),
							    str.size());
			}
		});
	}

	/* At this moment queue should be empty */
	bool consumed = queue.try_consume(
		[&](queue_type::read_accessor rd_acc) { ASSERT_UNREACHABLE; });
	UT_ASSERTeq(consumed, false);

	for (auto &v : values) {
		auto count = std::count(values_on_pmem.begin(),
					values_on_pmem.end(), v);
		UT_ASSERTeq(count, static_cast<int>(concurrency));
	}
	return 0;
}

int
main(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	constexpr size_t concurrency = 16;

	pmem::obj::pool<struct root> pop;

	pop = pmem::obj::pool<root>::create(
		std::string(path), LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	pmem::obj::transaction::run(pop, [&] {
		pop.root()->log =
			pmem::obj::make_persistent<queue_type::pmem_log_type>(
				QUEUE_SIZE);
	});

	return run_test([&] { mt_test(pop, concurrency); });
}
