// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "libpmemobj++/detail/ebr.hpp"
#include "radix.hpp"

/*
 * radix_concurrent_erase -- test erase on the radix_tree with one erasing
 * thread and multiple reading threads.
 */

static size_t INITIAL_ELEMENTS = 512;

static void
test_erase_find(nvobj::pool<root> &pop,
		nvobj::persistent_ptr<container_string> &ptr)
{
	const size_t value_repeats = 1000;
	size_t threads = 4;
	if (On_drd)
		threads = 2;

	init_container(pop, ptr, INITIAL_ELEMENTS, value_repeats);
	ptr->runtime_initialize_mt();

	auto allocs_before_erase = num_allocs(pop);

	auto writer_f = [&] {
		for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
			ptr->erase(key<container_string>(i));
			ptr->garbage_collect();
		}
	};

	auto readers_f = std::vector<std::function<void()>>{
		[&] {
			auto w = ptr->register_worker();

			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				w.critical([&] {
					auto res = ptr->find(
						key<container_string>(i));
					UT_ASSERT(
						res == ptr->end() ||
						res->value() ==
							value<container_string>(
								i,
								value_repeats));
				});
			}
		},
	};

	parallel_write_read(writer_f, readers_f, threads);

	/* check if something was removed permamently */
	// XXX
	UT_ASSERT(num_allocs(pop) <= allocs_before_erase + 3);

	/* call garbage_collect when other threads arent reading */
	for (size_t i = 0; i < 3; ++i) {
		ptr->garbage_collect();
	}
	UT_ASSERT(num_allocs(pop) <= 4);

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container_string>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

static void
test_write_erase_find(nvobj::pool<root> &pop,
		      nvobj::persistent_ptr<container_string> &ptr)
{
	const size_t value_repeats = 1000;
	size_t threads = 8;
	if (On_drd)
		threads = 4;

	init_container(pop, ptr, 0);
	ptr->runtime_initialize_mt();

	auto writer_f = [&] {
		for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
			ptr->emplace(key<container_string>(0),
				     value<container_string>(0, value_repeats));
			ptr->erase(key<container_string>(0));
		}
	};

	auto readers_f = std::vector<std::function<void()>>{
		[&] {
			auto w = ptr->register_worker();

			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				w.critical([&] {
					auto res = ptr->find(
						key<container_string>(0));
					UT_ASSERT(
						res == ptr->end() ||
						res->value() ==
							value<container_string>(
								0,
								value_repeats));
				});
			}
		},
	};

	parallel_write_read(writer_f, readers_f, threads);

	ptr->garbage_collect_force();

	UT_ASSERT(num_allocs(pop) <= 4);

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container_string>(ptr); });

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

	test_erase_find(pop, pop.root()->radix_str);
	test_write_erase_find(pop, pop.root()->radix_str);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
