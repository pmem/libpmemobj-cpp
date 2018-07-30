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

typedef persistent_map_type::value_type value_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> map1;
	nvobj::persistent_ptr<persistent_map_type> map2;
};

void
verify_elements(persistent_map_type &map, size_t elements)
{
	UT_ASSERT(map.size() == elements);

	for (int i = 0; i < static_cast<int>(elements); i++) {
		UT_ASSERT(map.count(i) == 1);
	}
}

/*
 * ctor_test -- (internal) test constrcutors
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
ctor_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	nvobj::make_persistent_atomic<persistent_map_type>(pop, map1, 10);
	UT_ASSERT(map1->bucket_count() >= 10);
	UT_ASSERT(map1->empty());

	for (int i = 0; i < 300; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	nvobj::make_persistent_atomic<persistent_map_type>(
		pop, map2, map1->begin(), map2->begin());

	UT_ASSERT(!map1->empty());
	UT_ASSERT(map1->size() == map2->size());

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	nvobj::make_persistent_atomic<persistent_map_type>(pop, map2, *map1);

	UT_ASSERT(map1->size() == map2->size());

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	nvobj::make_persistent_atomic<persistent_map_type>(pop, map2,
							   std::move(*map1));

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	nvobj::make_persistent_atomic<persistent_map_type>(
		pop, map2,
		std::initializer_list<value_type>{value_type(1, 1),
						  value_type(2, 2)});

	verify_elements(*map2, 2);

	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * assignment_test -- (internal) test assignment operators
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
assignment_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	nvobj::make_persistent_atomic<persistent_map_type>(pop, map1);
	nvobj::make_persistent_atomic<persistent_map_type>(pop, map2);

	UT_ASSERT(map1->empty());

	for (int i = 0; i < 50; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	for (int i = 0; i < 300; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	*map1 = *map2;

	verify_elements(*map1, 300);

	for (int i = 300; i < 350; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	verify_elements(*map1, 350);
	verify_elements(*map2, 300);

	map2->clear();

	*map1 = *map2;

	UT_ASSERT(map1->size() == 0);
	UT_ASSERT(std::distance(map1->begin(), map1->end()) == 0);
	UT_ASSERT(map2->size() == 0);
	UT_ASSERT(std::distance(map2->begin(), map2->end()) == 0);

	for (int i = 0; i < 350; i++) {
		UT_ASSERT(map1->count(i) == 0);
		UT_ASSERT(map2->count(i) == 0);
	}

	for (int i = 0; i < 100; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	verify_elements(*map1, 100);
	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * swap_test -- (internal) test swap method
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
swap_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	nvobj::make_persistent_atomic<persistent_map_type>(pop, map1);
	nvobj::make_persistent_atomic<persistent_map_type>(pop, map2);

	for (int i = 0; i < 50; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	for (int i = 0; i < 300; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	map1->swap(*map2);
	verify_elements(*map1, 300);
	verify_elements(*map2, 50);

	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * access_test -- (internal) test access methods
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
access_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;

	nvobj::make_persistent_atomic<persistent_map_type>(pop, map1);

	for (int i = 0; i < 100; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	UT_ASSERT(map1->begin() ==
		  static_cast<const persistent_map_type>(*map1).begin());
	UT_ASSERT(map1->end() ==
		  static_cast<const persistent_map_type>(*map1).end());

	int i = 0;
	auto it = map1->begin();
	auto const_it = static_cast<const persistent_map_type>(*map1).begin();
	while (it != map1->end()) {
		UT_ASSERT(it->first == i);
		UT_ASSERT(it->second == i);

		UT_ASSERT(const_it->first == i);
		UT_ASSERT(const_it->second == i);

		i++;
		it++;
		const_it++;
	}

	UT_ASSERT(static_cast<size_t>(i) == map1->size());

	pmem::detail::destroy<persistent_map_type>(*map1);
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
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	ctor_test(pop);
	assignment_test(pop);
	access_test(pop);
	swap_test(pop);

	pop.close();
}
