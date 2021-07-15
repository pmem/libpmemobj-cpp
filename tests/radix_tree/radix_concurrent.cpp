// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "radix.hpp"

/*
 * radix_concurrent -- test concurrent operations on the radix_tree (one writer,
 * multiple readers).
 */

static size_t INITIAL_ELEMENTS = 256;

template <typename Container>
static void
test_write_find(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	size_t threads = 8;
	if (On_drd)
		threads = 2;

	const size_t batch_size = INITIAL_ELEMENTS / threads;

	init_container(pop, ptr, INITIAL_ELEMENTS);
	ptr->runtime_initialize_mt();

	auto writer = [&]() {
		for (size_t i = INITIAL_ELEMENTS; i < INITIAL_ELEMENTS * 2;
		     ++i) {
			ptr->emplace(key<Container>(i), value<Container>(i));
		}
	};

	std::atomic<size_t> reader_id;
	reader_id.store(0);
	auto readers = std::vector<std::function<void()>>{
		[&]() {
			auto id = reader_id++;
			for (size_t i = id * batch_size;
			     i < (id + 1) * batch_size; ++i) {
				auto res = ptr->find(key<Container>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() == value<Container>(i));
			}
		},
	};

	parallel_modify_read(writer, readers, threads);

	ptr->runtime_finalize_mt();

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

/* this test only works with int as a key type */
static void
test_various_readers(nvobj::pool<root> &pop,
		     nvobj::persistent_ptr<cntr_int_int_mt> &ptr)
{
	size_t threads = 16;
	if (On_drd)
		threads = 4;

	init_container(pop, ptr, INITIAL_ELEMENTS);

	auto writer = [&]() {
		/* writer thread does nothing in this test */
	};

	auto readers = std::vector<std::function<void()>>{
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				auto res = ptr->find(key<cntr_int_int_mt>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() ==
					  value<cntr_int_int_mt>(i));
			}
		},
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				auto res = ptr->lower_bound(
					key<cntr_int_int_mt>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() ==
					  value<cntr_int_int_mt>(i));
			}
		},
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS - 1; ++i) {
				auto res =
					ptr->upper_bound(key<cntr_int_int>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() ==
					  value<cntr_int_int_mt>(i + 1));
			}
		},
	};

	parallel_modify_read(writer, readers, threads);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<cntr_int_int_mt>(ptr); });

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

	test_write_find(pop, pop.root()->radix_int_int_mt);
	test_various_readers(pop, pop.root()->radix_int_int_mt);

	if (!On_drd) {
		test_write_find(pop, pop.root()->radix_int_mt);
		test_write_find(pop, pop.root()->radix_int_str_mt);
		test_write_find(pop, pop.root()->radix_str_mt);
		test_write_find(pop,
				pop.root()->radix_inline_s_wchart_wchart_mt);
		test_write_find(pop, pop.root()->radix_inline_s_wchart_mt);
		test_write_find(pop, pop.root()->radix_inline_s_u8t_mt);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
