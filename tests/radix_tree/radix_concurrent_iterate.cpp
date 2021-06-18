// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include <algorithm>
#include <atomic>
#include <random>

#include "radix.hpp"

/*
 * radix_concurrent_iterate -- test multithread operations with radix's
 * iterators.
 */

static std::mt19937_64 generator;

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
		[&]() {
			size_t cnt = 0;
			for (auto it = --ptr->end(); it != ptr->end(); --it) {
				if (it->value() !=
				    value<Container>(INITIAL_ELEMENTS)) {
					++cnt;
				}
			}

			UT_ASSERTeq(cnt, INITIAL_ELEMENTS);
		}};

	parallel_write_read(writer, readers, threads);

	ptr->runtime_finalize_mt();
	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

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

	ptr->runtime_initialize_mt();

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
				auto it = ptr->lower_bound(i);
				UT_ASSERT(it->key() >=
					  key<container_int_int>(i));
			}
		},
		[&]() {
			auto id = reader_id++;
			for (size_t i = id * batch_size;
			     i < (id + 1) * batch_size; ++i) {
				auto it = ptr->upper_bound(i);
				UT_ASSERT(it->key() >
					  key<container_int_int>(i));
			}
		},
	};

	parallel_write_read(writer, readers, threads);

	ptr->runtime_finalize_mt();
	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container_int_int>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

void
test_erase_upper_lower_bounds_neighbours(
	nvobj::pool<root> &pop, nvobj::persistent_ptr<container_string> &ptr)
{
	const size_t value_repeats = 10;
	const size_t repeats = 100;
	size_t threads = 4;
	if (On_drd)
		threads = 2;

	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<container_string>(); });

	ptr->runtime_initialize_mt();

	const auto separator = "!!";
	for (size_t i = 0; i < INITIAL_ELEMENTS / 2; i++) {
		ptr->emplace(key<container_string>(i),
			     value<container_string>(i, value_repeats));
		ptr->emplace(key<container_string>(i) + separator +
				     key<container_string>(i),
			     value<container_string>(i, value_repeats));
	}

	/* Run this test for first, last, middle keys and a few non-existent
	 * ones */
	auto first = ptr->begin();
	auto first_key = std::string(first->key().data(), first->key().size());

	auto middle =
		std::next(ptr->begin(), static_cast<int>(ptr->size() / 2));
	auto middle_key =
		std::string(middle->key().data(), middle->key().size());
	middle_key.erase(
		std::find(middle_key.begin(), middle_key.end(), separator[0]),
		middle_key.end());

	auto last = ptr->rbegin();
	auto last_key = std::string(last->key().data(), last->key().size());
	last_key.erase(
		std::find(last_key.begin(), last_key.end(), separator[0]),
		last_key.end());

	std::vector<std::string> keys = {first_key,	   middle_key,
					 last_key,	   first_key + "0",
					 middle_key + "0", last_key + "0"};
	std::vector<std::string> extra_keys;
	for (auto &k : keys) {
		extra_keys.push_back(k + separator + k);
	}
	keys.insert(keys.begin(), extra_keys.begin(), extra_keys.end());

	for (auto &k : keys) {
		auto it = ptr->find(k);

		std::vector<std::string> keys_to_erase;
		keys_to_erase.push_back(k);
		keys_to_erase.push_back(k + separator + k);
		if (it != ptr->end() && std::next(it) != ptr->end()) {
			auto &tmp = std::next(it)->key();
			keys_to_erase.emplace_back(tmp.data(), tmp.size());
		}
		if (it != ptr->begin()) {
			auto &tmp = std::prev(it)->key();
			keys_to_erase.emplace_back(tmp.data(), tmp.size());
		}

		std::shuffle(keys_to_erase.begin(), keys_to_erase.end(),
			     generator);

		auto eraser = [&]() {
			for (auto &ke : keys_to_erase)
				ptr->erase(ke);
		};

		auto readers = std::vector<std::function<void()>>{
			[&] {
				for (size_t i = 0; i < repeats; i++) {
					auto lo = ptr->lower_bound(k);

					/* There is no element
						bigger/equal to k. */
					if (lo == ptr->end())
						UT_ASSERT(
							ptr->rbegin()->key() <
							pmem::obj::string_view(
								k));
					else
						UT_ASSERT(
							lo->key() >=
							pmem::obj::string_view(
								k));

					auto prev = std::prev(lo);

					/* There is no element smaller than k.
					 */
					if (prev == ptr->end())
						UT_ASSERT(
							ptr->begin()->key() >=
							pmem::obj::string_view(
								k));
					else
						UT_ASSERT(
							prev->key() <
							pmem::obj::string_view(
								k));
				}
			},
			[&] {
				for (size_t i = 0; i < repeats; i++) {
					auto ub = ptr->upper_bound(k);

					/* There is no element
					bigger than k. */
					if (ub == ptr->end())
						UT_ASSERT(
							ptr->rbegin()->key() <=
							pmem::obj::string_view(
								k));
					else
						UT_ASSERT(
							ub->key() >
							pmem::obj::string_view(
								k));

					auto prev = std::prev(ub);

					/* There is no element smaller than k.
					 */
					if (prev == ptr->end())
						UT_ASSERT(
							ptr->begin()->key() >
							pmem::obj::string_view(
								k));
					else
						UT_ASSERT(
							prev->key() <=
							pmem::obj::string_view(
								k));
				}
			}};

		parallel_write_read(eraser, readers, threads);
	}

	ptr->runtime_finalize_mt();
	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container_string>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

void
test_write_erase_upper_lower_bounds_split(
	nvobj::pool<root> &pop, nvobj::persistent_ptr<container_string> &ptr)
{
	const size_t value_repeats = 10;
	const size_t repeats = 100;
	size_t threads = 4;
	if (On_drd)
		threads = 2;

	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<container_string>(); });

	ptr->runtime_initialize_mt();

	const auto separator = "!!";
	const auto n_child = 9;

	/* Generate two-level tree. */
	for (size_t i = 0; i < n_child; i++) {
		ptr->emplace(key<container_string>(i),
			     value<container_string>(i, value_repeats));
		for (size_t j = 0; j < n_child; j++) {
			ptr->emplace(key<container_string>(i) + separator +
					     key<container_string>(j),
				     value<container_string>(j, value_repeats));
		}
	}

	auto number_of_elements = ptr->size();

	std::vector<size_t> key_nums_to_process = {0, n_child / 2, n_child - 1};
	std::vector<std::string> keys_to_process;
	for (auto &k : key_nums_to_process)
		keys_to_process.push_back(key<container_string>(k) + separator +
					  key<container_string>(k));

	for (auto &k : keys_to_process) {

		/* Erase and put back in key with "EXTRA" suffix. This should
		 * internally split and compress radix tree. */
		auto writer_eraser = [&] {
			for (size_t i = 0; i < repeats; i++) {
				auto ret = ptr->emplace(k + "EXTRA", k);
				UT_ASSERT(ret.second);
				UT_ASSERTeq(ptr->erase(k + "EXTRA"), 1);
			}
		};

		std::vector<std::string> keys_to_read = {k, k + "EXTRA",
							 k + "0", k + "EXTRA0"};
		auto readers = std::vector<std::function<void()>>{
			[&] {
				for (size_t i = 0; i < repeats; i++) {
					for (auto &k_read : keys_to_read) {
						auto lo = ptr->lower_bound(
							k_read);

						if (lo != ptr->end())
							UT_ASSERT(
								lo->key() >=
								pmem::obj::string_view(
									k_read));
					}
				}
			},
			[&] {
				for (size_t i = 0; i < repeats; i++) {
					for (auto &k_read : keys_to_read) {
						auto ub = ptr->upper_bound(
							k_read);

						if (ub != ptr->end())
							UT_ASSERT(
								ub->key() >
								pmem::obj::string_view(
									k_read));
					}
				}
			}};

		parallel_write_read(writer_eraser, readers, threads);

		UT_ASSERTeq(number_of_elements, ptr->size());
	}

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

	if (On_drd) {
		INITIAL_ELEMENTS = 64;
	}

	std::random_device rd;
	auto seed = rd();
	std::cout << "rand seed: " << seed << std::endl;
	generator = std::mt19937_64(seed);

	test_write_iterate(pop, pop.root()->radix_int_int);
	test_write_upper_lower_bounds(pop, pop.root()->radix_int_int);
	test_erase_upper_lower_bounds_neighbours(pop, pop.root()->radix_str);
	test_write_erase_upper_lower_bounds_split(pop, pop.root()->radix_str);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
