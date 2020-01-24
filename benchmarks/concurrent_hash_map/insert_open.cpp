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
 * insert_open.cpp -- this simple benchmarks is used to measure time of
 * inserting specified number of elements and time of runtime_initialize().
 */

#include <cassert>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../measure.hpp"

#ifndef _WIN32

#include <unistd.h>
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

#else

#include <windows.h>
#define CREATE_MODE_RW (S_IWRITE | S_IREAD)

#endif

static const std::string LAYOUT = "insert_open";

using key_type = pmem::obj::p<int>;
using value_type = pmem::obj::p<int>;

using persistent_map_type =
	pmem::obj::concurrent_hash_map<key_type, value_type>;

struct root {
	pmem::obj::persistent_ptr<persistent_map_type> pptr;
};

void
insert(pmem::obj::pool<root> &pop, size_t n_inserts, size_t n_threads)
{
	auto map = pop.root()->pptr;

	assert(map != nullptr);

	map->runtime_initialize();

	std::vector<std::thread> v;
	for (size_t i = 0; i < n_threads; i++) {
		v.emplace_back(
			[&](size_t tid) {
				int begin = tid * n_inserts;
				int end = begin + int(n_inserts);
				for (int i = begin; i < end; ++i) {
					persistent_map_type::value_type val(i,
									    i);
					map->insert(val);
				}
			},
			i);
	}

	for (auto &t : v)
		t.join();

	assert(map->size() == n_inserts * n_threads);
}

void
open(pmem::obj::pool<root> &pop)
{
	auto map = pop.root()->pptr;

	assert(map != nullptr);

	map->runtime_initialize();

	assert(map->size() > 0);
}

int
main(int argc, char *argv[])
{
	std::string usage =
		"usage: %s file-name <create n_inserts n_threads | open>";

	if (argc < 3) {
		std::cerr << usage << std::endl;
		return 1;
	}

	auto mode = std::string(argv[2]);

	if (mode != "create" && mode != "open") {
		std::cerr << usage << std::endl;
		return 1;
	}

	if (mode == "create" && argc < 5) {
		std::cerr << usage << std::endl;
		return 1;
	}

	const char *path = argv[1];

	pmem::obj::pool<root> pop;

	if (mode == "create") {
		size_t n_inserts = std::stoull(argv[3]);
		size_t n_threads = std::stoull(argv[4]);

		if (n_inserts * n_threads == 0) {
			std::cerr << "n_inserts and n_threads must be > 0";
			return 1;
		}

		try {
			auto pool_size =
				n_inserts * n_threads * sizeof(int) * 65 +
				20 * PMEMOBJ_MIN_POOL;

			pop = pmem::obj::pool<root>::create(
				path, LAYOUT, pool_size, CREATE_MODE_RW);
			pmem::obj::transaction::run(pop, [&] {
				pop.root()->pptr = pmem::obj::make_persistent<
					persistent_map_type>();
			});
		} catch (pmem::pool_error &pe) {
			std::cerr << "!pool::create: " << pe.what()
				  << std::endl;
			return 1;
		}

		std::cout << measure<std::chrono::milliseconds>([&] {
			insert(pop, n_inserts, n_threads);
		}) << "ms" << std::endl;
	} else {
		try {
			pop = pmem::obj::pool<root>::open(path, LAYOUT);
		} catch (pmem::pool_error &pe) {
			std::cerr << "!pool::open: " << pe.what() << std::endl;
			return 1;
		}

		std::cout << measure<std::chrono::milliseconds>([&] {
			open(pop);
		}) << "ms" << std::endl;
	}

	pop.close();
	return 0;
}
