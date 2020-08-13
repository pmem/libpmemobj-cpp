// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/atomic_self_relative_ptr.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>

#include <memory>
#include <vector>

constexpr size_t CONCURRENCY = 20;
constexpr size_t MEAN_CONCURRENCY = CONCURRENCY * 2;
constexpr size_t HIGH_CONCURRENCY = CONCURRENCY * 5;

using pmem::obj::experimental::self_relative_ptr;
using std::atomic;

void
test_fetch()
{
	constexpr auto count_iterations = 500;
	constexpr auto arr_size = CONCURRENCY * count_iterations;
	std::vector<int> vptr(arr_size, 0);
	atomic<self_relative_ptr<int>> ptr;
	ptr = vptr.data();
	parallel_exec(CONCURRENCY, [&](size_t) {
		for (size_t i = 0; i < count_iterations; ++i) {
			auto element = ptr.fetch_add(1);
			*element += 1;
		}
	});
	UT_ASSERT(vptr.data() + arr_size == ptr.load().get());

	parallel_exec(CONCURRENCY, [&](size_t) {
		for (size_t i = 0; i < count_iterations; ++i) {
			auto element = ptr.fetch_sub(1) - 1;
			*element += 1;
		}
	});
	UT_ASSERT(vptr.data() == ptr.load().get());

	for (auto element : vptr) {
		UT_ASSERTeq(element, 2);
	}
}

void
test_exchange()
{
	self_relative_ptr<int> first = new int(10);
	self_relative_ptr<int> second = new int(20);

	std::atomic<self_relative_ptr<int>> ptr;

	UT_ASSERT(ptr.load(std::memory_order_acquire).is_null());

	ptr.store(first, std::memory_order_release);

	UT_ASSERTeq(*ptr.load(), *first);

	auto before_ptr = ptr.exchange(second, std::memory_order_acq_rel);

	UT_ASSERTeq(*before_ptr, *first);
	UT_ASSERTeq(*ptr.load(std::memory_order_acquire), *second);

	parallel_exec(MEAN_CONCURRENCY, [&](size_t i) {
		for (size_t j = 0; j < 1000000; j++) {
			auto before = ptr.exchange(i % 2 == 0 ? first : second,
						   std::memory_order_acq_rel);
			UT_ASSERT(before == first || before == second);
		}
	});

	auto last_ptr = ptr.load();
	UT_ASSERT(last_ptr == first || last_ptr == second);

	delete first.get();
	delete second.get();
}

void
test_compare_exchange()
{
	int *first = new int(4);
	int *second = new int(5);
	std::atomic<self_relative_ptr<int>> atomic_ptr;
	self_relative_ptr<int> tst_val{first}, new_val{second};
	bool exchanged;

	// tst_val != atomic_ptr  ==>  tst_val is modified
	exchanged = atomic_ptr.compare_exchange_strong(tst_val, new_val);

	UT_ASSERT(exchanged == false);
	UT_ASSERT(tst_val == nullptr);

	// tst_val == atomic_ptr  ==>  atomic_ptr is modified
	exchanged = atomic_ptr.compare_exchange_strong(tst_val, new_val);

	UT_ASSERT(exchanged == true);
	UT_ASSERT(tst_val == nullptr);
	UT_ASSERT(atomic_ptr.load() == new_val);
	UT_ASSERT(*atomic_ptr.load() == *new_val);

	delete first;
	delete second;
}

static void
test(int argc, char *argv[])
{
	test_fetch();
	test_exchange();
	test_compare_exchange();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
