// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * recovery_after_consume.cpp -- pmreorder test for mpsc_queue
 * which breaks produce after produce/consume cycle.
 */

#include "../queue.hpp"

#include <algorithm>
#include <string>

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
	pmem::obj::p<size_t> capacity;
};

static constexpr size_t QUEUE_SIZE = 3000;
static const auto produce_size = 128ULL;
static const size_t concurrency = 4;
const auto fill_pattern = std::string(produce_size, 'x');

void
run_consistent(pmem::obj::pool<root> pop)
{
	const auto produce_size = 128ULL;
	const size_t concurrency = 4;
	const auto fill_pattern = std::string(produce_size, 'x');

	auto proot = pop.root();
	auto queue = queue_type(*proot->log, concurrency);

	bool consumed = queue.try_consume_batch(
		[&](queue_type::batch_type rd_acc) { ASSERT_UNREACHABLE; });
	UT_ASSERTeq(consumed, false);

	for (size_t i = 0; i < concurrency; i++)
		proot->written[i] = 0;

	size_t capacity = get_queue_capacity(queue);
	UT_ASSERTne(capacity, 0);

	proot->capacity = capacity;
	pop.persist(proot->capacity);

	make_queue_with_first_half_empty(queue);
	/* Run this under pmreorder. After crash state of the queue should be
	 * something like this: | produced | crashed | produced | empty |
	 * produced |
	 */
	parallel_xexec(
		concurrency, [&](size_t id, std::function<void()> syncthreads) {
			auto worker = queue.register_worker();

			if (id == 0)
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.BEGIN");

			auto ret = worker.try_produce(fill_pattern);

			if (id == 0)
				VALGRIND_PMC_EMIT_LOG("PMREORDER_MARKER.END");

			proot->written[id] = 1;
			pop.persist(proot->written[id]);

			UT_ASSERT(ret);
		});
}

void
check_consistency(pmem::obj::pool<root> pop)
{
	auto proot = pop.root();
	auto queue = queue_type(*proot->log, concurrency);

	size_t expected = proot->capacity / 2;
	for (size_t i = 0; i < MAX_CONCURRENCY; i++) {
		if (proot->written[i])
			expected++;
	}

	std::vector<std::string> values_on_pmem;
	queue.try_consume_batch([&](queue_type::batch_type rd_acc) {
		for (auto entry : rd_acc)
			values_on_pmem.emplace_back(entry.data(), entry.size());
	});

	UT_ASSERT(values_on_pmem.size() >= expected);

	for (auto &str : values_on_pmem) {
		UT_ASSERT(str == fill_pattern);
	}

	auto worker = queue.register_worker();

	static const auto overwrite_size = 64;
	static const auto overwrite_pattern = std::string(overwrite_size, 'y');

	while (true) {
		auto ret = worker.try_produce(overwrite_pattern);

		if (!ret)
			break;
	}

	values_on_pmem.clear();
	auto ret = queue.try_consume_batch([&](queue_type::batch_type rd_acc) {
		for (auto entry : rd_acc)
			values_on_pmem.emplace_back(entry.data(), entry.size());
	});
	UT_ASSERT(ret);

	for (auto &str : values_on_pmem) {
		UT_ASSERT(str == overwrite_pattern);
	}
}

void
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
	if (argc != 3 || strchr("cox", argv[1][0]) == nullptr)
		UT_FATAL("usage: %s <c|o|x> file-name", argv[0]);

	const char *path = argv[2];

	pmem::obj::pool<root> pop;

	try {
		if (argv[1][0] == 'o') {
			pop = pmem::obj::pool<root>::open(path, LAYOUT);

			check_consistency(pop);
		} else if (argv[1][0] == 'c') {
			pop = pmem::obj::pool<root>::create(
				path, LAYOUT, PMEMOBJ_MIN_POOL * 20,
				S_IWUSR | S_IRUSR);

			init(pop);
		} else if (argv[1][0] == 'x') {
			pop = pmem::obj::pool<root>::open(path, LAYOUT);

			run_consistent(pop);
		}
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
