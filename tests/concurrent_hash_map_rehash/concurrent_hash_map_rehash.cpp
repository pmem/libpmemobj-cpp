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
 * concurrent_hash_map.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <thread>
#include <vector>

#include <libpmemobj++/experimental/concurrent_hash_map.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

typedef nvobj::experimental::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

/*
 * insert_and_erase_test -- (internal) test insert and erase operations
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_erase_lookup_test(nvobj::pool<root> &pop)
{
	const size_t NUMBER_ITEMS_INSERT = 500;

	// Adding more concurrency will increase DRD test time
	const size_t concurrency = 6;

	auto map = pop.root()->cons;

	UT_ASSERT(map != nullptr);

	map->initialize();

	std::vector<std::thread> threads;
	threads.reserve(concurrency);

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				map->insert(
					persistent_map_type::value_type(i, i));
			}
		});
	}

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				map->erase(i);
			}
		});
	}

	for (size_t i = 0; i < concurrency; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0;
			     i < static_cast<int>(NUMBER_ITEMS_INSERT); ++i) {
				persistent_map_type::accessor acc;
				bool res = map->find(acc, i);

				if (res) {
					UT_ASSERTeq(acc->first, i);
					UT_ASSERT(acc->second >= i);
					acc->second.get_rw() += 1;
					pop.persist(acc->second);
				}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	for (auto &e : *map) {
		UT_ASSERT(e.first <= e.second);
	}
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
		nvobj::make_persistent_atomic<persistent_map_type>(
			pop, pop.root()->cons);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	insert_erase_lookup_test(pop);

	pop.close();

	return 0;
}
