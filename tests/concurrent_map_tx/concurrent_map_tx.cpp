// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * concurrent_hash_map_tx.cpp -- pmem::obj::experimental::concurrent_map test
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

#include <libpmemobj++/experimental/concurrent_map.hpp>

#define LAYOUT "concurrent_map"

namespace nvobj = pmem::obj;

typedef nvobj::experimental::concurrent_map<nvobj::p<int>, nvobj::p<int>>
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

	using value_type = typename persistent_map_type::value_type;
	using key_type = typename persistent_map_type::key_type;

	auto map = pop.root()->map;

	map->runtime_initialize();

	pmem::obj::transaction::run(pop, [&] {
		value_type v(0, 0);
		assert_tx_exception([&] { (void)map->insert(v); });

		assert_tx_exception(
			[&] { (void)map->insert(std::pair<int, int>(0, 0)); });

		assert_tx_exception(
			[&] { (void)map->insert(value_type(0, 0)); });

		assert_tx_exception([&] {
			(void)map->insert(map->end(), value_type(0, 0));
		});

		assert_tx_exception([&] {
			(void)map->insert(map->end(),
					  std::pair<int, int>(0, 0));
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

		assert_tx_exception([&] { (void)map->emplace(0, 0); });

		assert_tx_exception(
			[&] { (void)map->emplace_hint(map->end(), 0, 0); });

		key_type k(0);
		assert_tx_exception([&] { (void)map->try_emplace(k, 0); });

		assert_tx_exception(
			[&] { (void)map->try_emplace(key_type(0), 0); });

		assert_tx_exception([&] { (void)map->try_emplace(0, 0); });

		assert_tx_exception([&] { (void)map->unsafe_erase(0); });

		assert_tx_exception(
			[&] { (void)map->unsafe_erase(map->begin()); });

		assert_tx_exception([&] {
			(void)map->unsafe_erase(map->begin(), map->end());
		});
	});

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
		auto it = map->find(i);
		auto it2 = map2->find(i);

		UT_ASSERT(it->second == i);
		UT_ASSERT(it2->second == i + 1);
	}
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

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
