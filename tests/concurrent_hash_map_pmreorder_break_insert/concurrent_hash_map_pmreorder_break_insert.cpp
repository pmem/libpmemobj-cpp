/*
 * Copyright 2019, Intel Corporation
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
 * concurrent_hash_map_pmreorder_break_insert.cpp --
 * pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <future>
#include <iostream>

#include <libpmemobj++/container/concurrent_hash_map.hpp>

#define LAYOUT "persistent_concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

typedef nvobj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

typedef persistent_map_type::value_type value_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> cons;
};

static constexpr int elements[] = {
	1,       /* bucket #1 */
	2,       /* bucket #2 */
	3,       /* bucket #3 */
	2 + 255, /* bucket #1 */
	3 + 255, /* bucket #2 */
	4 + 255, /* bucket #3 */
};

static constexpr int len_elements =
	static_cast<int>(sizeof(elements) / sizeof(elements[0]));

/*
 * check_exist -- (internal) check existence of an element
 */
void
check_exist(nvobj::persistent_ptr<persistent_map_type> &map, int element,
	    bool exists)
{
	typename persistent_map_type::accessor accessor;

	UT_ASSERTeq(map->find(accessor, element), exists);

	if (exists) {
		UT_ASSERTeq(accessor->first, element);
		UT_ASSERTeq(accessor->second, element);
	}
}

/*
 * test_init -- (internal) init test
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
test_init(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;
	persistent_map->runtime_initialize();

	for (int i = 0; i < len_elements / 2; i++) {
		persistent_map->insert(value_type(elements[i], elements[i]));
		check_exist(persistent_map, elements[i], true);
	}
}

/*
 * test_insert -- (internal) test
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
test_insert(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;
	persistent_map->runtime_initialize();

	for (int i = len_elements / 2; i < len_elements - 1; i++) {
		persistent_map->insert(value_type(elements[i], elements[i]));
		check_exist(persistent_map, elements[i], true);
	}
}

void
check_consistency(nvobj::pool<root> &pop)
{
	auto persistent_map = pop.root()->cons;
	persistent_map->runtime_initialize();

	auto size = static_cast<typename persistent_map_type::difference_type>(
		persistent_map->size());

	UT_ASSERTeq(
		std::distance(persistent_map->begin(), persistent_map->end()),
		size);

	for (int i = 0; i < size; i++) {
		UT_ASSERTeq(persistent_map->count(elements[i]), 1);
		check_exist(persistent_map, elements[i], true);
	}

	for (int i = size; i < len_elements; i++)
		UT_ASSERTeq(persistent_map->count(elements[i]), 0);
}
}

int
main(int argc, char *argv[])
{
	if (argc != 3 || strchr("coi", argv[1][0]) == nullptr)
		UT_FATAL("usage: %s <c|o|i> file-name", argv[0]);

	const char *path = argv[2];

	nvobj::pool<root> pop;

	try {
		if (argv[1][0] == 'o') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			check_consistency(pop);
		} else if (argv[1][0] == 'c') {
			pop = nvobj::pool<root>::create(path, LAYOUT,
							PMEMOBJ_MIN_POOL * 20,
							S_IWUSR | S_IRUSR);

			pmem::obj::transaction::run(pop, [&] {
				pop.root()->cons = nvobj::make_persistent<
					persistent_map_type>();
			});

			test_init(pop);
		} else if (argv[1][0] == 'i') {
			pop = nvobj::pool<root>::open(path, LAYOUT);

			test_insert(pop);
		}
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	pop.close();

	return 0;
}
