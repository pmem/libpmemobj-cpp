// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "thread_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/atomic_self_relative_ptr.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>

#include <algorithm>
#include <memory>
#include <vector>

constexpr size_t CONCURRENCY = 20;
constexpr size_t MEAN_CONCURRENCY = CONCURRENCY * 2;
constexpr size_t HIGH_CONCURRENCY = CONCURRENCY * 5;

using pmem::obj::experimental::self_relative_ptr;

template <typename T, bool need_volatile>
using atomic_type = typename std::conditional<
	need_volatile,
	typename std::add_volatile<std::atomic<self_relative_ptr<T>>>::type,
	std::atomic<self_relative_ptr<T>>>::type;

template <bool volatile_atomic = false>
void
test_fetch()
{
	constexpr auto count_iterations = 300;
	constexpr auto arr_size = CONCURRENCY * count_iterations;
	std::vector<int> vptr(arr_size, 0);

	atomic_type<int, volatile_atomic> ptr;

	ptr = vptr.data();
	parallel_exec(CONCURRENCY, [&](size_t) {
		for (size_t i = 0; i < count_iterations; ++i) {
			auto element = ptr.fetch_add(1);
			*element += 1;
		}
	});

	for (auto element : vptr) {
		UT_ASSERTeq(element, 1);
	}
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

	parallel_exec(CONCURRENCY, [&](size_t) {
		for (size_t i = 0; i < count_iterations; ++i) {
			auto element = ptr++;
			*element += 1;
		}
	});

	for (auto element : vptr) {
		UT_ASSERTeq(element, 3);
	}
	UT_ASSERT(vptr.data() + arr_size == ptr.load().get());

	parallel_exec(CONCURRENCY, [&](size_t) {
		for (size_t i = 0; i < count_iterations; ++i) {
			auto element = --ptr;
			*element += 1;
		}
	});
	UT_ASSERT(vptr.data() == ptr.load().get());

	for (auto element : vptr) {
		UT_ASSERTeq(element, 4);
	}
}

template <bool volatile_atomic = false>
void
test_exchange()
{
	self_relative_ptr<int> first = new int(10);
	self_relative_ptr<int> second = new int(20);

	atomic_type<int, volatile_atomic> ptr;

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

/**
 * Small lock free stack for tests
 */
template <bool volatile_atomic = false>
class test_stack {
public:
	struct node;

	using value_type = size_t;
	using node_ptr_type = self_relative_ptr<node>;

	struct node {
		size_t value;
		node_ptr_type next;
	};

	void
	push(const value_type &data)
	{
		node_ptr_type new_node = new node{data, nullptr};

		auto next_node = head.load(std::memory_order_relaxed);

		while (!head.compare_exchange_weak(next_node, new_node,
						   std::memory_order_release,
						   std::memory_order_relaxed))
			; // empty
		new_node->next = next_node;
	}

	std::vector<value_type>
	get_all()
	{
		auto current_node = head.load();
		std::vector<value_type> values;
		while (current_node != nullptr) {
			values.push_back(current_node->value);
			current_node = current_node->next;
		}
		return values;
	}

	~test_stack()
	{
		auto current_node = head.load();
		while (current_node != nullptr) {
			auto prev_node = current_node.get();
			current_node = current_node->next;
			delete prev_node;
		}
	}

private:
	atomic_type<node, volatile_atomic> head;
};

template <bool volatile_atomic = false>
void
test_compare_exchange()
{
	int *first = new int(4);
	int *second = new int(5);
	atomic_type<int, volatile_atomic> atomic_ptr{first};
	std::atomic<size_t> exchanged(0);

	parallel_exec(CONCURRENCY, [&](size_t) {
		// tst_val != atomic_ptr  ==>  tst_val is modified
		// tst_val == atomic_ptr  ==>  atomic_ptr is modified

		self_relative_ptr<int> tst_val{first}, new_val{second};
		if (atomic_ptr.compare_exchange_strong(tst_val, new_val)) {
			++exchanged;
		} else {
			UT_ASSERT(tst_val == new_val);
		}
	});

	UT_ASSERTeq(exchanged.load(), 1);
	UT_ASSERT(atomic_ptr.load().get() == second);
	UT_ASSERT(*atomic_ptr.load() == *second);

	delete first;
	delete second;

	test_stack<volatile_atomic> stack;
	constexpr size_t count_iterations = 1000;
	parallel_exec(HIGH_CONCURRENCY, [&stack](size_t i) {
		for (size_t j = 0; j < count_iterations; j++) {
			stack.push(j + (i * count_iterations));
		}
	});
	auto all = stack.get_all();
	std::sort(all.begin(), all.end());
	for (size_t i = 0; i < HIGH_CONCURRENCY * count_iterations; i++) {
		UT_ASSERTeq(all[i], i);
	}
}

static void
test(int argc, char *argv[])
{
	test_fetch();
	test_exchange();
	test_compare_exchange();

	test_fetch<true>();
	test_exchange<true>();
	test_compare_exchange<true>();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
