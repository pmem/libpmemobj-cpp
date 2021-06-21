// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * recovery.cpp -- pmreorder test for mpsc_queue
 * which breaks produce.
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

static constexpr size_t MAX_CONCURRENCY = 100;

struct root {
	pmem::obj::persistent_ptr<queue_type::pmem_log_type> log;
	pmem::obj::p<size_t> written[MAX_CONCURRENCY];
};

static constexpr size_t QUEUE_SIZE = 10000;
static const auto produce_size = 128ULL;
static const size_t concurrency = 4;
static const auto fill_pattern = char(1);

/* Break application during produce. */
static void
run_consistent(pmem::obj::pool<root> pop, bool break_produce, bool synchronized)
{
	auto proot = pop.root();
	auto queue = queue_type(*proot->log, concurrency);

	auto ret = queue.try_consume_batch(
		[&](queue_type::batch_type acc) { ASSERT_UNREACHABLE; });
	UT_ASSERT(!ret);

	for (size_t i = 0; i < MAX_CONCURRENCY; i++)
		proot->written[i] = 0;

	parallel_xexec(
		concurrency, [&](size_t id, std::function<void()> syncthreads) {
			auto worker = queue.register_worker();

			if (id == 0 && break_produce)
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.BEGIN");

			worker.try_produce(produce_size,
					   [&](pmem::obj::slice<char *> range) {
						   if (synchronized) {
							   /* Make sure that all
							    * other threads
							    * called
							    * ringbuf_acquire.
							    */
							   syncthreads();
						   }

						   std::fill_n(range.begin(),
							       produce_size,
							       fill_pattern);
					   });

			if (id == 0 && break_produce)
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.END");

			proot->written[id] = 1;
			pop.persist(proot->written[id]);
		});
}

static void
check_consistency(pmem::obj::pool<root> pop, bool already_consumed)
{
	auto proot = pop.root();
	auto queue = queue_type(*proot->log, concurrency);

	size_t expected = 0;
	for (size_t i = 0; i < MAX_CONCURRENCY; i++) {
		if (proot->written[i])
			expected++;
	}

	std::vector<std::string> values_on_pmem;
	queue.try_consume_batch([&](queue_type::batch_type rd_acc) {
		for (auto entry : rd_acc)
			values_on_pmem.emplace_back(entry.data(), entry.size());
	});

	if (already_consumed) {
		UT_ASSERT(values_on_pmem.size() <= expected);
	} else
		UT_ASSERT(values_on_pmem.size() >= expected);

	for (auto &str : values_on_pmem) {
		UT_ASSERT(str == std::string(produce_size, fill_pattern));
	}
}

static void
run_break_recovery(pmem::obj::pool<root> pop)
{
	auto proot = pop.root();
	auto queue = queue_type(*proot->log, concurrency);

	size_t expected = 0;
	for (size_t i = 0; i < MAX_CONCURRENCY; i++) {
		if (proot->written[i])
			expected++;
	}

	VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.BEGIN");

	std::vector<std::string> values_on_pmem;
	queue.try_consume_batch([&](queue_type::batch_type rd_acc) {
		for (auto entry : rd_acc)
			values_on_pmem.emplace_back(entry.data(), entry.size());
	});

	VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.BEGIN");

	UT_ASSERT(values_on_pmem.size() >= expected);

	for (auto &str : values_on_pmem) {
		UT_ASSERT(str == std::string(produce_size, fill_pattern));
	}
}

static void
init(pmem::obj::pool<root> &pop)
{
	pmem::obj::transaction::run(pop, [&] {
		pop.root()->log =
			pmem::obj::make_persistent<queue_type::pmem_log_type>(
				QUEUE_SIZE);
	});
}

static void
test(int argc, char *argv[])
{
	if (argc < 3 || strchr("cox", argv[1][0]) == nullptr)
		UT_FATAL(
			"usage: %s <c|o|x> break_recovery file-name [synchronized]",
			argv[0]);

	auto break_recovery = std::stoi(argv[2]);
	const char *path = argv[3];

	pmem::obj::pool<root> pop;

	try {
		if (argv[1][0] == 'o') {
			pop = pmem::obj::pool<root>::open(path, LAYOUT);

			check_consistency(pop, break_recovery);
		} else if (argv[1][0] == 'c') {
			pop = pmem::obj::pool<root>::create(
				path, LAYOUT, PMEMOBJ_MIN_POOL * 20,
				S_IWUSR | S_IRUSR);

			init(pop);
		} else if (argv[1][0] == 'x') {
			pop = pmem::obj::pool<root>::open(path, LAYOUT);
			auto synchronized = std::stoi(argv[4]);

			run_consistent(pop, !break_recovery, synchronized);

			if (break_recovery)
				run_break_recovery(pop);
		}
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
