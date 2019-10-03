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
 * concurrent_hash_map.cpp -- C++ documentation snippets.
 */

//! [concurrent_hash_map_example]
#include <iostream>
#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <vector>

using namespace pmem::obj;

// In this example we will be using concurrent_hash_map with p<int> type for
// both keys and values
using hashmap_type = concurrent_hash_map<p<int>, p<int>>;

const int THREADS_NUM = 30;

// This is basic example and we only need to use concurrent_hash_map. Hence we
// will correlate memory pool root object with single instance of persistent
// pointer to hasmap_type
struct root {
	persistent_ptr<hashmap_type> pptr;
};

int
main(int argc, char *argv[])
{
	if (argc != 2)
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;

	auto path = argv[1];

	auto pop = pool<root>::open(path, "concurrent_hash_map example");
	auto r = pop.root();

	if (r->pptr == nullptr) {
		// Logic when file didn't exist when open() was called. After
		// the pool was created, we have to allocate object of
		// hashmap_type and attach it to the root object.
		pmem::obj::transaction::run(pop, [&] {
			r->pptr = make_persistent<hashmap_type>();
		});
	} else {
		// Logic when file already exists. After opening of the pool we
		// have to call runtime_initialize() function in order to
		// recalculate mask and check for consistentcy.

		pop.root()->pptr->runtime_initialize();
	}

	auto &map = *pop.root()->pptr;

	std::vector<std::thread> threads;
	threads.reserve(static_cast<size_t>(THREADS_NUM));

	// Insert THREADS_NUM / 3 key-value pairs to the hashmap. This operation
	// is thread-safe.
	for (int i = 0; i < THREADS_NUM / 3; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0; i < 10 * THREADS_NUM; ++i) {
				map.insert(hashmap_type::value_type(i, i));
			}
		});
	}

	// Erase THREADS_NUM /3 key-value pairs from the hashmap. This operation
	// is thread-safe.
	for (int i = 0; i < THREADS_NUM / 3; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0; i < 10 * THREADS_NUM; ++i) {
				map.erase(i);
			}
		});
	}

	// Check if given key is in the hashmap. For the time of an accessor
	// life, the read-write lock is taken on the item.
	for (int i = 0; i < THREADS_NUM / 3; ++i) {
		threads.emplace_back([&]() {
			for (int i = 0; i < 10 * THREADS_NUM; ++i) {
				hashmap_type::accessor acc;
				bool res = map.find(acc, i);

				if (res) {
					assert(acc->first == i);
					assert(acc->second >= i);
					acc->second.get_rw() += 1;
					pop.persist(acc->second);
				}
			}
		});
	}

	for (auto &t : threads) {
		t.join();
	}

	// Erase remaining itemes in map. This function is not thread-safe,
	// hence the function is being called only after thread execution has
	// completed.
	map.clear();

	pop.close();

	return 0;
}

//! [concurrent_hash_map_example]
