/*
 * Copyright 2020, Intel Corporation
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
 * concurrent_hash_map_insert_reopen.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "../concurrent_hash_map/concurrent_hash_map_test.hpp"
#include "unittest.hpp"

/*
 * insert_reopen_test -- (internal) test insert operations and verify
 * consistency after reopen
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_reopen_test(nvobj::pool<root> &pop, std::string path,
		   size_t concurrency = 4)
{
	PRINT_TEST_PARAMS;

	size_t thread_items = 50;

	{
		ConcurrentHashMapTestPrimitives test(
			pop, concurrency * thread_items);

		auto map = pop.root()->cons;

		UT_ASSERT(map != nullptr);

		map->runtime_initialize();

		parallel_exec(concurrency, [&](size_t thread_id) {
			int begin = thread_id * thread_items;
			int end = begin + int(thread_items);
			for (int i = begin; i < end; ++i) {
				persistent_map_type::value_type val(i, i);
				test.insert<persistent_map_type::accessor>(val);
			}
		});

		test.check_items_count();

		pop.close();
	}

	{
		size_t already_inserted_num = concurrency * thread_items;

		pop = nvobj::pool<root>::open(path, LAYOUT);

		ConcurrentHashMapTestPrimitives test(
			pop, concurrency * thread_items);

		auto map = pop.root()->cons;

		UT_ASSERT(map != nullptr);

		map->runtime_initialize();

		test.check_items_count();

		parallel_exec(concurrency, [&](size_t thread_id) {
			int begin = thread_id * thread_items;
			int end = begin + int(thread_items);
			for (int i = begin; i < end; ++i) {
				persistent_map_type::value_type val(
					i + int(already_inserted_num), i);
				test.insert<persistent_map_type::accessor>(val);
			}
		});

		test.check_items_count(already_inserted_num * 2);
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

	size_t concurrency = 8;
	if (On_drd)
		concurrency = 2;
	std::cout << "Running tests for " << concurrency << " threads"
		  << std::endl;

	insert_reopen_test(pop, path, concurrency);

	pop.close();
	return 0;
}
