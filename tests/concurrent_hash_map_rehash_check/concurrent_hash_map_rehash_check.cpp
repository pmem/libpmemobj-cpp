/*
 * Copyright 2018-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * concurrent_hash_map_rehash_check.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <thread>
#include <vector>

#include <libpmemobj++/container/concurrent_hash_map.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

typedef nvobj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

static constexpr size_t CONCURRENCY = 4;

void
check_elements(nvobj::pool<root> &pop, size_t number_items_insert)
{
	auto map = pop.root()->cons;

	for (int i = 0; i < static_cast<int>(number_items_insert); ++i) {
		UT_ASSERTeq(map->count(i), 1);

		persistent_map_type::accessor acc;
		UT_ASSERTeq(map->find(acc, i), true);

		UT_ASSERTeq(acc->first, i);
		UT_ASSERTeq(acc->second, i);
	}
}

void
run_inserts(nvobj::pool<root> &pop, size_t from, size_t number_items_insert)
{
	std::vector<std::thread> threads;
	threads.reserve(CONCURRENCY);

	auto map = pop.root()->cons;

	for (size_t i = 0; i < CONCURRENCY; ++i) {
		threads.emplace_back([&]() {
			for (int i = static_cast<int>(from);
			     i < static_cast<int>(from + number_items_insert);
			     ++i) {
				map->insert(
					persistent_map_type::value_type(i, i));
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}
}

/*
 * rehash_test -- (internal) test rehash operation and verify all elements
 * are accessible.
 */
void
rehash_test(nvobj::pool<root> &pop)
{
	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->runtime_initialize();

	run_inserts(pop, 0, 100);

	map->rehash(1024);
	check_elements(pop, 100);

	run_inserts(pop, 100, 2048);

	map->rehash(1024 * (1 << 1));
	check_elements(pop, 2148);

	run_inserts(pop, 2148, 100);

	map->rehash(1024 * (1 << 3));
	check_elements(pop, 2248);
}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->cons =
				nvobj::make_persistent<persistent_map_type>();
		});
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	rehash_test(pop);

	pop.close();

	return 0;
}
