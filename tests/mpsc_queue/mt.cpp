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
static constexpr size_t QUEUE_SIZE = 3 * pmem::detail::CACHELINE_SIZE;

struct root {
	pmem::obj::persistent_ptr<queue_type::pmem_log_type> log;
};

/* basic multithreaded for produce-consume */
static void
mt_test(pmem::obj::pool<root> pop, size_t concurrency)
{
	auto proot = pop.root();

	auto queue = queue_type(*proot->log, concurrency);

	bool consumed = queue.try_consume_batch(
		[&](queue_type::batch_type rd_acc) { ASSERT_UNREACHABLE; });
	UT_ASSERTeq(consumed, false);

	std::vector<std::string> values = {"xxx", "aaaaaaa", "bbbbb", "cccc"};

	std::atomic<size_t> threads_counter(concurrency);
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	VALGRIND_HG_DISABLE_CHECKING(&threads_counter, sizeof(threads_counter));
#endif

	std::vector<std::string> values_on_pmem;
	parallel_exec(concurrency + 1, [&](size_t thread_id) {
		if (thread_id == 0) {
			/* Read data while writting */
			while (threads_counter.load() > 0) {
				queue.try_consume_batch(
					[&](pmem::obj::experimental::
						    mpsc_queue::batch_type
							    rd_acc) {
						for (auto str : rd_acc) {
							values_on_pmem.emplace_back(
								str.data(),
								str.size());
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
						e,
						[&](pmem::obj::string_view
							    target) {
							UT_ASSERT(
								pmem::obj::string_view(
									e) ==
								target);
							x++;
						});
				};
			}
			UT_ASSERTeq(x, values.size());
			threads_counter--;
		}
	});

	/* Consume the rest of the data. */
	queue.try_consume_batch([&](queue_type::batch_type rd_acc1) {
		for (auto str : rd_acc1) {
			values_on_pmem.emplace_back(str.data(), str.size());
		}
	});

	/* At this moment queue should be empty */
	consumed = queue.try_consume_batch(
		[&](queue_type::batch_type rd_acc) { ASSERT_UNREACHABLE; });
	UT_ASSERTeq(consumed, false);

	for (auto &v : values) {
		auto count = std::count(values_on_pmem.begin(),
					values_on_pmem.end(), v);
		UT_ASSERTeq(count, static_cast<int>(concurrency));
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	size_t concurrency = 48;
	if (On_valgrind)
		concurrency = 2;

	pmem::obj::pool<struct root> pop;

	pop = pmem::obj::pool<root>::create(
		std::string(path), LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	pmem::obj::transaction::run(pop, [&] {
		pop.root()->log =
			pmem::obj::make_persistent<queue_type::pmem_log_type>(
				QUEUE_SIZE);
	});

	mt_test(pop, concurrency);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
