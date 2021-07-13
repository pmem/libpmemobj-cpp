// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "radix.hpp"

/*
 * radix_concurrent_erase -- test erase on the radix_tree with one erasing
 * thread and multiple reading threads.
 */

static size_t INITIAL_ELEMENTS = 512;

/* Insert INITAL_ELEMENTS elements to the radix. After that concurrently try to
 * erase all elements and read them from the other threads. */
static void
test_erase_find(nvobj::pool<root> &pop,
		nvobj::persistent_ptr<container_string_mt> &ptr)
{
	const size_t value_repeats = 1000;
	size_t threads = 4;
	if (On_drd)
		threads = 2;

	init_container(pop, ptr, INITIAL_ELEMENTS, value_repeats);
	ptr->runtime_initialize_mt();

	auto erase_f = [&] {
		for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
			ptr->erase(key<container_string_mt>(i));
			ptr->garbage_collect();
		}
	};

	auto readers_f = std::vector<std::function<void()>>{
		[&] {
			auto w = ptr->register_worker();

			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				w.critical([&] {
					auto res = ptr->find(
						key<container_string_mt>(i));
					UT_ASSERT(
						res == ptr->end() ||
						res->value() ==
							value<container_string_mt>(
								i,
								value_repeats));
				});
			}
		},
	};

	parallel_modify_read(erase_f, readers_f, threads);

	ptr->garbage_collect_force();
	UT_ASSERT(num_allocs(pop) <= 4);

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string_mt>(ptr);
	});

	UT_ASSERTeq(num_allocs(pop), 0);
}

/* operator-- does not work well when MtMode is enabled */

/* Insert INITAL_ELEMENTS elements to the radix. After that concurrently try to
 * erase some at the end and read them (and decrement) from the other threads.
 */
// static void
// test_erase_decrement(nvobj::pool<root> &pop,
//		     nvobj::persistent_ptr<container_int_int_mt> &ptr)
// {
//	size_t threads = 4;
//	if (On_drd)
//		threads = 2;

//	init_container(pop, ptr, INITIAL_ELEMENTS);
//	ptr->runtime_initialize_mt();

//	std::vector<size_t> to_erase;

//	/* order randomly elements to remove */
//	auto it = ptr->begin();
//	while (it != ptr->end()) {
//		to_erase.emplace_back(it->key());
//		++it;
//	}
//	std::shuffle(to_erase.begin(), to_erase.end(), generator);

//	auto erase_f = [&] {
//		for (size_t i = 0; i < to_erase.size(); ++i) {
//			ptr->erase(key<container_int_int_mt>(to_erase[i]));
//			ptr->garbage_collect();
//		}
//	};

//	auto readers_f = std::vector<std::function<void()>>{
//		[&] {
//			auto w = ptr->register_worker();

//			for (size_t i = INITIAL_ELEMENTS - 1; i > 1; --i) {
//				/* reading is slower than erasing - repeat it */
//				for (int j = 0; j < 15; ++j) {
//					w.critical([&] {
//						auto k = key<
//							container_int_int_mt>(
//							i);
//						auto v = value<
//							container_int_int_mt>(
//							i);
//						auto it = ptr->find(k);
//						UT_ASSERT(it == ptr->end() ||
//							  it->value() == v);
//						if (it != ptr->end()) {
//							auto prev = --it;
//							if (prev != ptr->end())
//								UT_ASSERT(
//									prev->value() <
//									v);
//						}
//					});
//				}
//			}
//		},
//	};

//	parallel_modify_read(erase_f, readers_f, threads);

//	ptr->garbage_collect_force();
//	UT_ASSERT(num_allocs(pop) <= 5);

//	ptr->runtime_finalize_mt();

//	nvobj::transaction::run(pop, [&] {
//		nvobj::delete_persistent<container_int_int_mt>(ptr);
//	});

//	UT_ASSERTeq(num_allocs(pop), 0);
// }

/* Insert INITAL_ELEMENTS elements to the radix. After that concurrently try to
 * erase some at the beginning and read them (and increment) from the other
 * threads.
 */
static void
test_erase_increment(nvobj::pool<root> &pop,
		     nvobj::persistent_ptr<container_int_int_mt> &ptr)
{
	size_t threads = 4;
	if (On_drd)
		threads = 2;

	init_container(pop, ptr, INITIAL_ELEMENTS);
	ptr->runtime_initialize_mt();

	auto erase_f = [&] {
		for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
			ptr->erase(key<container_int_int_mt>(i));
			ptr->garbage_collect();
		}
	};

	auto readers_f = std::vector<std::function<void()>>{
		[&] {
			auto w = ptr->register_worker();

			/* start one element ahead */
			for (size_t i = 1; i < INITIAL_ELEMENTS - 1; ++i) {
				w.critical([&] {
					auto k = key<container_int_int_mt>(i);
					auto v = value<container_int_int_mt>(i);
					auto it = ptr->find(k);
					UT_ASSERT(it == ptr->end() ||
						  it->value() == v);
					if (it != ptr->end()) {
						auto next = ++it;
						UT_ASSERT(next != ptr->end());
						UT_ASSERT(next->key() > k);
					}
				});
			}
		},
	};

	parallel_modify_read(erase_f, readers_f, threads);

	ptr->garbage_collect_force();
	UT_ASSERT(num_allocs(pop) <= 4);

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_int_int_mt>(ptr);
	});

	UT_ASSERTeq(num_allocs(pop), 0);
}

/* Insert and erase the same element in loop for INITAL_ELEMENTS times.
 * Concurrently try to read this element from other threads */
static void
test_write_erase_find(nvobj::pool<root> &pop,
		      nvobj::persistent_ptr<container_string_mt> &ptr)
{
	const size_t value_repeats = 1000;
	size_t threads = 8;
	if (On_drd)
		threads = 4;

	init_container(pop, ptr, 0);
	ptr->runtime_initialize_mt();

	auto writer_f = [&] {
		for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
			ptr->emplace(
				key<container_string_mt>(0),
				value<container_string_mt>(0, value_repeats));
			ptr->erase(key<container_string_mt>(0));
			ptr->garbage_collect();
		}
	};

	auto readers_f = std::vector<std::function<void()>>{
		[&] {
			auto w = ptr->register_worker();

			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				w.critical([&] {
					auto res = ptr->find(
						key<container_string_mt>(0));
					UT_ASSERT(
						res == ptr->end() ||
						res->value() ==
							value<container_string_mt>(
								0,
								value_repeats));
				});
			}
		},
	};

	parallel_modify_read(writer_f, readers_f, threads);

	ptr->garbage_collect_force();

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string_mt>(ptr);
	});

	UT_ASSERTeq(num_allocs(pop), 0);
}

/* Test if radix->garbage_collect() is able to free some memory. To ensure that
 * the test won't fail randomly (there can be a situation where nothing is
 * possible to delete permanently), some synchronization is added between
 * deleting and reading elements */
static void
test_garbage_collection(nvobj::pool<root> &pop,
			nvobj::persistent_ptr<container_string_mt> &ptr)
{
	const size_t value_repeats = 1000;
	size_t threads = 8;
	if (On_drd)
		threads = 4;

	init_container(pop, ptr, INITIAL_ELEMENTS, value_repeats);
	ptr->runtime_initialize_mt();

	auto allocs_before_erase = num_allocs(pop);

	parallel_xexec(threads, [&](size_t id, std::function<void(void)> syncthreads) {
		if (id == 0) {
			/* deleter */
			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				ptr->erase(key<container_string_mt>(i));

				if (i % 50 == 0) {
					syncthreads();
					ptr->garbage_collect();
					syncthreads();
				}
			}
		} else {
			/* reader */
			auto w = ptr->register_worker();

			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				w.critical([&] {
					auto res = ptr->find(
						key<container_string_mt>(i));
					UT_ASSERT(
						res == ptr->end() ||
						res->value() ==
							value<container_string_mt>(
								i,
								value_repeats));
				});
				if (i % 50 == 0) {
					syncthreads();
					syncthreads();
				}
			}
		}
	});

	/* check if something was removed permanently */
	UT_ASSERT(num_allocs(pop) < allocs_before_erase);

	ptr->garbage_collect_force();

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string_mt>(ptr);
	});

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

	test_erase_find(pop, pop.root()->radix_str_mt);
	// test_erase_decrement(pop, pop.root()->radix_int_int_mt);
	test_erase_increment(pop, pop.root()->radix_int_int_mt);
	test_write_erase_find(pop, pop.root()->radix_str_mt);
	test_garbage_collection(pop, pop.root()->radix_str_mt);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
