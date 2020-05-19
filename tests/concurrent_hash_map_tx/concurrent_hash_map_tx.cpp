// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * concurrent_hash_map_tx.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <array>
#include <iterator>
#include <thread>
#include <vector>

#include <libpmemobj++/container/concurrent_hash_map.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

typedef nvobj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> map;
	nvobj::persistent_ptr<persistent_map_type> map2;
};

/*
 * It verifies that f() throws transaction_scope_error exception.
 */
void
assert_tx_exception(std::function<void(void)> f)
{
	bool exception_thrown = false;
	try {
		f();
		UT_ASSERT(0);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
}

void
test_tx_exception(nvobj::pool<root> &pop)
{
	pmem::obj::transaction::run(pop, [&] {
		pop.root()->map = nvobj::make_persistent<persistent_map_type>();
	});

	auto map = pop.root()->map;

	map->runtime_initialize();

	pmem::obj::transaction::run(pop, [&] {
		assert_tx_exception([&] { (void)map->count(0); });

		assert_tx_exception([&] {
			persistent_map_type::const_accessor acc;
			(void)map->find(acc, 0);
		});

		assert_tx_exception([&] {
			persistent_map_type::accessor acc;
			(void)map->find(acc, 0);
		});

		assert_tx_exception([&] {
			persistent_map_type::const_accessor acc;
			(void)map->insert(acc, 0);
		});

		assert_tx_exception([&] {
			persistent_map_type::accessor acc;
			(void)map->insert(acc, 0);
		});

		assert_tx_exception([&] {
			persistent_map_type::const_accessor acc;
			(void)map->insert(
				acc, persistent_map_type::value_type(0, 0));
		});

		assert_tx_exception([&] {
			persistent_map_type::accessor acc;
			(void)map->insert(
				acc, persistent_map_type::value_type(0, 0));
		});

		persistent_map_type::value_type val(0, 0);

		assert_tx_exception([&] { (void)map->insert(val); });

		assert_tx_exception([&] {
			persistent_map_type::const_accessor acc;
			(void)map->insert(acc, val);
		});

		assert_tx_exception([&] {
			persistent_map_type::accessor acc;
			(void)map->insert(acc, val);
		});

		assert_tx_exception([&] {
			persistent_map_type::accessor acc;
			(void)map->insert(acc, val);
		});

		std::array<persistent_map_type::value_type, 2> arr{
			persistent_map_type::value_type{0, 0},
			persistent_map_type::value_type{1, 1}};

		assert_tx_exception(
			[&] { (void)map->insert(arr.begin(), arr.end()); });

		assert_tx_exception([&] {
			(void)map->insert(
				{persistent_map_type::value_type{0, 0},
				 persistent_map_type::value_type{1, 1}});
		});

		assert_tx_exception([&] { (void)map->erase(0); });

		assert_tx_exception([&] { map->rehash(); });
	});

	persistent_map_type::value_type val(0, 0);

	{
		persistent_map_type::accessor acc;
		map->insert(acc, val);

		pmem::obj::transaction::run(pop, [&] {
			assert_tx_exception([&] { acc.release(); });
		});
	}

	pmem::obj::transaction::run(pop, [&] {
		pmem::obj::delete_persistent<persistent_map_type>(map);
	});
}

void
verify_elements(nvobj::pool<root> &pop, int number_of_inserts)
{
	auto map = pop.root()->map;
	auto map2 = pop.root()->map2;

	for (int i = 0; i < number_of_inserts; i++) {
		persistent_map_type::accessor acc1, acc2;
		map->find(acc1, i);
		map2->find(acc2, i);

		UT_ASSERT(acc1->second == i);
		UT_ASSERT(acc2->second == i + 1);
	}
}

void
test_tx_singlethread(nvobj::pool<root> &pop)
{
	pmem::obj::transaction::run(pop, [&] {
		pop.root()->map = nvobj::make_persistent<persistent_map_type>();
	});

	auto number_of_inserts = 100;

	auto map = pop.root()->map;
	map->runtime_initialize();

	pmem::obj::transaction::run(pop, [&] {
		pop.root()->map2 =
			nvobj::make_persistent<persistent_map_type>();

		auto map_tmp = nvobj::make_persistent<persistent_map_type>();
		auto map_tmp2 = nvobj::make_persistent<persistent_map_type>(
			std::move(*map_tmp));

		nvobj::delete_persistent<persistent_map_type>(map_tmp);
		nvobj::delete_persistent<persistent_map_type>(map_tmp2);
	});

	auto map2 = pop.root()->map2;

	for (int i = 0; i < number_of_inserts; i++) {
		map->insert(persistent_map_type::value_type(i, i));
		map2->insert(persistent_map_type::value_type(i, i + 1));
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			map->swap(*map2);
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	verify_elements(pop, number_of_inserts);

	try {
		pmem::obj::transaction::run(pop, [&] {
			*map = *map2;
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	auto bucket_count = map->bucket_count();

	try {
		pmem::obj::transaction::run(pop, [&] {
			map->clear();
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(bucket_count, map->bucket_count());
	verify_elements(pop, number_of_inserts);

	try {
		pmem::obj::transaction::run(pop, [&] {
			map->clear();
			*map = {{0, 0}};
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(bucket_count, map->bucket_count());
	verify_elements(pop, number_of_inserts);

	try {
		pmem::obj::transaction::run(pop, [&] {
			*map = {{0, 0}, {1, 1}};
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(bucket_count, map->bucket_count());
	verify_elements(pop, number_of_inserts);

	try {
		pmem::obj::transaction::run(pop, [&] {
			for (auto &e : *map) {
				e.second = 10;
			}
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(bucket_count, map->bucket_count());
	verify_elements(pop, number_of_inserts);

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<persistent_map_type>(map);
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(bucket_count, map->bucket_count());
	verify_elements(pop, number_of_inserts);

	auto test_value = 10;
	{
		persistent_map_type::accessor acc;
		map->find(acc, test_value);

		try {
			pmem::obj::transaction::run(pop, [&] {
				UT_ASSERT(acc->second == test_value);
				acc->second = 0;
				UT_ASSERT(acc->second == 0);

				pmem::obj::transaction::abort(0);
			});
		} catch (pmem::manual_tx_abort &) {
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	verify_elements(pop, number_of_inserts);

	{
		persistent_map_type::accessor acc;
		map->find(acc, test_value);

		UT_ASSERT(acc->second == test_value);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			map->clear();
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(static_cast<int>(map->size()) == number_of_inserts);

	try {
		pmem::obj::transaction::run(pop, [&] {
			map->free_data();
			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	verify_elements(pop, number_of_inserts);

	try {
		pmem::obj::transaction::run(pop, [&] {
			map->free_data();
			pmem::obj::delete_persistent<persistent_map_type>(map);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	pmem::obj::transaction::run(pop, [&] {
		pmem::obj::delete_persistent<persistent_map_type>(map2);
	});
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_tx_exception(pop);

	test_tx_singlethread(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
