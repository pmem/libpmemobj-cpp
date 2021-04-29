// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "radix.hpp"

/*
 * radix_concurrent -- test concurrent operations on the radix_tree (one writer,
 * multiple readers).
 */

static size_t INITIAL_ELEMENTS = 256;

template <typename WriteF, typename ReadF>
static void
parallel_write_read(WriteF writer, std::vector<ReadF> &readers,
		    size_t n_readers)
{
	parallel_exec(n_readers + 1, [&](size_t thread_id) {
		if (thread_id == 0) {
			writer();
		} else {
			readers[(thread_id - 1) % readers.size()]();
		}
	});
}

template <typename Container>
static void
init_container(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<Container>(); });

	for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
		ptr->emplace(key<Container>(i), value<Container>(i));
	}
}

template <typename Container>
static void
test_write_find(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	const size_t threads = 16;
	const size_t batch_size = INITIAL_ELEMENTS / threads;

	init_container(pop, ptr);

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

	parallel_write_read(writer, readers, threads);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });
}

// XXX: For now, we cannot concurrently read and overwrite values
#if FALSE
template <typename Container>
static void
test_overwrite_find(nvobj::pool<root> &pop,
		    nvobj::persistent_ptr<Container> &ptr)
{
	const size_t threads = 16;

	init_container(pop, ptr);

	auto writer = [&]() {
		for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
			ptr->insert_or_assign(key<Container>(i),
					      value<Container>(i + 1));
		}
	};

	auto readers = std::vector<std::function<void()>>{
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				auto res = ptr->find(key<Container>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() == value<Container>(i) ||
					  res->value() ==
						  value<Container>(i + 1));
			}
		},
	};

	parallel_write_read(writer, readers, threads);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });
}

static void
test_overwrite_bigger_size_find(
	nvobj::pool<root> &pop,
	nvobj::persistent_ptr<container_int_string> &ptr)
{
	const size_t threads = 16;
	const std::string new_val = std::string(100, 'a');

	init_container(pop, ptr);

	auto writer = [&]() {
		for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
			ptr->insert_or_assign(key<container_int_string>(i),
					      new_val);
		}
	};

	auto readers = std::vector<std::function<void()>>{
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				auto res =
					ptr->find(key<container_int_string>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() ==
						  value<container_int_string>(
							  i) ||
					  res->value() == new_val);
			}
		},
	};

	parallel_write_read(writer, readers, threads);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_int_string>(ptr);
	});
}
#endif

template <typename Container>
static void
test_various_readers(nvobj::pool<root> &pop,
		     nvobj::persistent_ptr<Container> &ptr)
{
	const size_t threads = 12;

	init_container(pop, ptr);

	auto writer = [&]() {
		/* writer thread does nothing in this test */
	};

	auto readers = std::vector<std::function<void()>>{
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				auto res = ptr->find(key<Container>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() == value<Container>(i));
			}
		},
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS; ++i) {
				auto res = ptr->lower_bound(key<Container>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() == value<Container>(i));
			}
		},
		[&]() {
			for (size_t i = 0; i < INITIAL_ELEMENTS - 1; ++i) {
				auto res = ptr->upper_bound(key<Container>(i));
				UT_ASSERT(res != ptr->end());
				UT_ASSERT(res->value() ==
					  value<Container>(i + 1));
			}
		},
	};

	parallel_write_read(writer, readers, threads);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });
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

	test_write_find(pop, pop.root()->radix_int_int);
	// test_overwrite_find(pop, pop.root()->radix_int_int);
	/* this test only works with int as a value type */
	test_various_readers(pop, pop.root()->radix_int_int);

	test_write_find(pop, pop.root()->radix_int);
	// test_overwrite_find(pop, pop.root()->radix_int);

	test_write_find(pop, pop.root()->radix_int_str);
	// test_overwrite_find(pop, pop.root()->radix_int_str);
	// test_overwrite_bigger_size_find(pop, pop.root()->radix_int_str);

	test_write_find(pop, pop.root()->radix_str);
	// test_overwrite_find(pop, pop.root()->radix_str);

	test_write_find(pop, pop.root()->radix_inline_s_wchart_wchart);
	// test_overwrite_find(pop, pop.root()->radix_inline_s_wchart_wchart);

	test_write_find(pop, pop.root()->radix_inline_s_wchart);
	// test_overwrite_find(pop, pop.root()->radix_inline_s_wchart);

	test_write_find(pop, pop.root()->radix_inline_s_u8t);
	// test_overwrite_find(pop, pop.root()->radix_inline_s_u8t);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
