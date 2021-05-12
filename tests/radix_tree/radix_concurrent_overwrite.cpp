// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "radix.hpp"

/*
 * radix_concurrent -- test concurrent overwrite operations on the radix_tree.
 */

static size_t INITIAL_ELEMENTS = 256;

static void
test_overwrite_bigger_size_find(
	nvobj::pool<root> &pop,
	nvobj::persistent_ptr<container_int_string> &ptr)
{
	size_t threads = 16;
	if (On_drd)
		threads = 2;

	init_container(pop, ptr, INITIAL_ELEMENTS);
	ptr->runtime_initialize_mt();

	/* Overwrites initial elements with string(i, 100) */
	auto writer = [&]() {
		for (size_t i = 0; i < INITIAL_ELEMENTS * 2; ++i) {
			auto k = i % INITIAL_ELEMENTS;
			ptr->insert_or_assign(
				key<container_int_string>(k),
				value<container_int_string>(i, 100));
		}
	};

	auto readers = std::vector<std::function<void()>>{
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS * 2; ++i) {
				auto k = i % INITIAL_ELEMENTS;
				auto res =
					ptr->find(key<container_int_string>(k));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() ==
						  value<container_int_string>(
							  k) ||
					  res->value() ==
						  value<container_int_string>(
							  k, 100) ||
					  res->value() ==
						  value<container_int_string>(
							  k + INITIAL_ELEMENTS,
							  100));
			}
		},
	};

	parallel_write_read(writer, readers, threads);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_int_string>(ptr);
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

	test_overwrite_bigger_size_find(pop, pop.root()->radix_int_str);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
