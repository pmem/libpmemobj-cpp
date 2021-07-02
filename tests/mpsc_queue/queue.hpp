// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/experimental/mpsc_queue.hpp>

using queue_type = pmem::obj::experimental::mpsc_queue;

/* Returns capacity in bytes */
size_t
get_queue_capacity(queue_type &q)
{
	auto worker = q.register_worker();

	size_t capacity = 0;
	auto e = std::string("b");
	/* Check how many elements fit in the log. */
	while (worker.try_produce("b")) {
		capacity++;
	}

	/* Clear the queue */
	auto ret = q.try_consume_batch([](queue_type::batch_type acc) {
		for (const auto &e : acc)
			(void)e;
	});
	UT_ASSERT(ret);

	return capacity;
}

void
make_queue_with_first_half_empty(queue_type &q)
{
	auto worker = q.register_worker();
	auto capacity = get_queue_capacity(q);

	size_t produced = 0;
	while (produced < capacity) {
		/* Produce half of the elements, call consume and
		 * produce the rest. This should result in log being
		 * consumed at the
		 * beginning and unconsumed at the end. */
		auto ret = worker.try_produce("x");
		produced++;
		UT_ASSERT(ret);

		if (produced == capacity / 2) {
			size_t cnt_consumed = 0;
			auto ret = q.try_consume_batch(
				[&](pmem::obj::experimental::mpsc_queue::
					    batch_type acc) {
					for (const auto &str : acc) {
						(void)str;
						cnt_consumed++;
					}
				});
			UT_ASSERT(ret);
			UT_ASSERTeq(cnt_consumed, produced);
		}
	}

	UT_ASSERTeq(capacity, produced);
}
