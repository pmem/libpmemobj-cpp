/*
 * Copyright 2018, Intel Corporation
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
 * persistent_concurrent_hash_map.cpp -- tbb persistent concurrent hash map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <tbb/parallel_for.h>

#include <libpmemobj++/experimental/persistent_concurrent_hash_map.hpp>

#include <tbb/concurrent_hash_map.h>

#define LAYOUT "persistent_concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

const int NUMBER_ITEMS_INSERT = 1000;

typedef nvobj::experimental::persistent_concurrent_hash_map<nvobj::p<int>,
							    nvobj::p<int>>
	persistent_map_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

/*
 * test_map -- (internal) test
 * tbb::persistent_concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
test_map(nvobj::pool<root> &pop, bool open)
{
	auto persistent_map = pop.get_root()->cons;

	UT_ASSERT(persistent_map != nullptr);

	tbb::parallel_for(
		tbb::blocked_range<int>(0, NUMBER_ITEMS_INSERT),
		[&](const tbb::blocked_range<int> &range) {
			for (int i = range.begin(); i < range.end(); ++i) {
				bool ret = persistent_map->insert(
					persistent_map_type::value_type(i, i));
				UT_ASSERT(ret == true);
			}
		});

	UT_ASSERT(std::distance(persistent_map->begin(),
				persistent_map->end()) == NUMBER_ITEMS_INSERT);

	tbb::parallel_for(
		tbb::blocked_range<int>(0, NUMBER_ITEMS_INSERT),
		[&](const tbb::blocked_range<int> &range) {
			for (int i = range.begin(); i < range.end(); ++i) {
				UT_ASSERT(persistent_map->count(i) == 1);
			}
		});

	persistent_map->clear();

	tbb::parallel_for(
		tbb::blocked_range<int>(0, NUMBER_ITEMS_INSERT),
		[&](const tbb::blocked_range<int> &range) {
			for (int i = range.begin(); i < range.end(); ++i) {
				UT_ASSERT(persistent_map->count(i) == 0);
			}
		});

	UT_ASSERT(std::distance(persistent_map->begin(),
				persistent_map->end()) == 0);
}
}

int
main(int argc, char *argv[])
{
	if (argc != 3 || strchr("co", argv[1][0]) == nullptr)
		UT_FATAL("usage: %s <c,o> file-name", argv[0]);

	const char *path = argv[2];

	nvobj::pool<root> pop;
	bool open = (argv[1][0] == 'o');

	try {
		if (open) {
			pop = nvobj::pool<root>::open(path, LAYOUT);

		} else {
			pop = nvobj::pool<root>::create(path, LAYOUT,
							PMEMOBJ_MIN_POOL * 20,
							S_IWUSR | S_IRUSR);
			nvobj::make_persistent_atomic<persistent_map_type>(
				pop, pop.get_root()->cons);
		}
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_map(pop, open);

	// Test that persistent_concurrent_hash_map could co-exist with
	// volatile tbb::concurrent_hash_map
	tbb::concurrent_hash_map<int, int> my_map;

	my_map.insert(std::pair<int, int>(1, 1));

	pop.close();
}
