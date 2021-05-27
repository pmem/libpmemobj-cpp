// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include <atomic>

#include "libpmemobj++/detail/ebr.hpp"
#include "radix.hpp"

/*
 * radix_concurrent_iterate -- test multithread operations with radix's
 * iterators.
 */

static size_t INITIAL_ELEMENTS = 256;

/* Insert INITIAL_ELEMENTS elements to the radix. After that, concurrently
 * insert another INITIAL_ELEMENTS elements with special values and iterate
 * through the entire container to count elements with the value !=
 * special_value */
template <typename Container>
void
test_write_iterate(nvobj::pool<root> &pop,
		   nvobj::persistent_ptr<Container> &ptr)
{
	size_t threads = 8;
	if (On_drd)
		threads = 2;

	init_container(pop, ptr, INITIAL_ELEMENTS);
	ptr->runtime_initialize_mt();

	auto writer = [&]() {
		for (size_t i = INITIAL_ELEMENTS; i < INITIAL_ELEMENTS * 2;
		     ++i) {
			ptr->emplace(key<Container>(i),
				     value<Container>(INITIAL_ELEMENTS));
		}
	};

	auto readers = std::vector<std::function<void()>>{
		[&]() {
			size_t cnt = 0;
			for (auto it = ptr->begin(); it != ptr->end(); ++it) {
				if (it->value() !=
				    value<Container>(INITIAL_ELEMENTS)) {
					++cnt;
				}
			}
			UT_ASSERTeq(cnt, INITIAL_ELEMENTS);
		},
	};

	parallel_write_read(writer, readers, threads);

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

/* Insert INITIAL_ELEMENTS elements to the radix. After that, concurrently
 * erase elements with even keys and iterate through the entire container to
 * count elements with odd keys. */
void
test_erase_iterate(nvobj::pool<root> &pop,
		   nvobj::persistent_ptr<container_int_int> &ptr)
{
	const size_t value_repeats = 1000;
	size_t threads = 4;
	if (On_drd)
		threads = 2;

	init_container(pop, ptr, INITIAL_ELEMENTS, value_repeats);
	for (size_t i = 0; i < INITIAL_ELEMENTS; i += 2) {
		ptr->erase(key<container_int_int>(i));
	}
	auto expected_allocs = num_allocs(pop);
	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container_int_int>(ptr); });

	init_container(pop, ptr, INITIAL_ELEMENTS, value_repeats);
	ptr->runtime_initialize_mt();

	auto writer_f = [&] {
		for (size_t i = 0; i < INITIAL_ELEMENTS; i += 2) {
			ptr->erase(key<container_int_int>(i));
		}
	};

	auto readers_f = std::vector<std::function<void()>>{
		[&] {
			auto w = ptr->register_worker();

			size_t cnt = 0;
			w.critical([&] {
				for (auto it = ptr->begin(); it != ptr->end();
				     ++it) {
					if (it->key() % 2) {
						++cnt;
					}
				}
			});
			UT_ASSERTeq(cnt, INITIAL_ELEMENTS / 2);
		},
	};

	parallel_write_read(writer_f, readers_f, threads);

	UT_ASSERTeq(ptr->size(), INITIAL_ELEMENTS / 2);

	ptr->garbage_collect_force();

	/* It is random how many garbage vectors will be allocated inside the
	 * radix (from 0 to 3) */
	UT_ASSERT(num_allocs(pop) >= expected_allocs &&
		  num_allocs(pop) <= expected_allocs + 3);

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container_int_int>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

void
test_write_upper_lower_bounds(nvobj::pool<root> &pop,
			      nvobj::persistent_ptr<container_int_int> &ptr)
{
	const size_t value_repeats = 10;
	size_t threads = 4;
	if (On_drd)
		threads = 2;
	const size_t batch_size = INITIAL_ELEMENTS / threads;

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<container_int_int>();
	});

	for (size_t i = 0; i < 2 * INITIAL_ELEMENTS; i += 2) {
		ptr->emplace(key<container_int_int>(i),
			     value<container_int_int>(i, value_repeats));
	}

	auto writer = [&]() {
		for (size_t i = 1; i < 2 * INITIAL_ELEMENTS; i += 2) {
			ptr->emplace(
				key<container_int_int>(i),
				value<container_int_int>(INITIAL_ELEMENTS));
		}
	};

	std::atomic<size_t> reader_id;
	reader_id.store(0);
	auto readers = std::vector<std::function<void()>>{
		[&]() {
			auto id = reader_id++;
			for (size_t i = id * batch_size;
			     i < (id + 1) * batch_size; ++i) {
				std::vector<unsigned int> keys;
				auto it = ptr->lower_bound(i);
				while (it != ptr->end()) {
					keys.push_back(it->key());
					++it;
				}

				for (auto &k : keys) {
					UT_ASSERT(k >=
						  key<container_int_int>(i));
				}
			}
		},
		[&]() {
			auto id = reader_id++;
			for (size_t i = id * batch_size;
			     i < (id + 1) * batch_size; ++i) {
				std::vector<unsigned int> keys;
				auto it = ptr->upper_bound(i);
				while (it != ptr->end()) {
					keys.push_back(it->key());
					++it;
				}

				for (auto &k : keys) {
					UT_ASSERT(k >
						  key<container_int_int>(i));
				}
			}
		},
	};

	parallel_write_read(writer, readers, threads);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container_int_int>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

void
test_erase_upper_lower_bounds(nvobj::pool<root> &pop,
			      nvobj::persistent_ptr<container_int_int> &ptr)
{
	const size_t value_repeats = 10;
	size_t threads = 4;
	if (On_drd)
		threads = 2;
	const size_t batch_size = INITIAL_ELEMENTS / threads;

	init_container(pop, ptr, INITIAL_ELEMENTS, value_repeats);

	auto writer = [&]() {
		for (size_t i = 0; i < INITIAL_ELEMENTS; i += 2) {
			ptr->erase(key<container_int_int>(i));
		}
	};

	std::atomic<size_t> reader_id;
	reader_id.store(0);
	auto readers = std::vector<std::function<void()>>{
		[&]() {
			auto id = reader_id++;
			for (size_t i = id * batch_size;
			     i < (id + 1) * batch_size; ++i) {
				std::vector<unsigned int> keys;
				auto it = ptr->lower_bound(i);
				while (it != ptr->end()) {
					keys.push_back(it->key());
					++it;
				}

				for (auto &k : keys) {
					UT_ASSERT(k >=
						  key<container_int_int>(i));
				}
			}
		},
		[&]() {
			auto id = reader_id++;
			for (size_t i = id * batch_size;
			     i < (id + 1) * batch_size; ++i) {
				std::vector<unsigned int> keys;
				auto it = ptr->upper_bound(i);
				while (it != ptr->end()) {
					keys.push_back(it->key());
					++it;
				}

				for (auto &k : keys) {
					UT_ASSERT(k >
						  key<container_int_int>(i));
				}
			}
		},
	};

	parallel_write_read(writer, readers, threads);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container_int_int>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<struct root>::create(path, "radix_concurrent",
						       10 * PMEMOBJ_MIN_POOL,
						       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	if (On_drd) {
		INITIAL_ELEMENTS = 64;
	}

	test_write_iterate(pop, pop.root()->radix_int_int);
	test_erase_iterate(pop, pop.root()->radix_int_int);
	test_write_upper_lower_bounds(pop, pop.root()->radix_int_int);
	test_erase_upper_lower_bounds(pop, pop.root()->radix_int_int);

	if (!On_drd) {
		test_write_iterate(pop, pop.root()->radix_int);
		test_write_iterate(pop, pop.root()->radix_int_str);
		test_write_iterate(pop, pop.root()->radix_str);
		test_write_iterate(pop,
				   pop.root()->radix_inline_s_wchart_wchart);
		test_write_iterate(pop, pop.root()->radix_inline_s_wchart);
		test_write_iterate(pop, pop.root()->radix_inline_s_u8t);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
