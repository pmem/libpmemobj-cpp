// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

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
	pmem::obj::pool<root> pop;
	try {
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

		if (mode == "create") {
			size_t n_inserts = std::stoull(argv[3]);
			size_t n_threads = std::stoull(argv[4]);

			if (n_inserts * n_threads == 0) {
				std::cerr
					<< "n_inserts and n_threads must be > 0";
				return 1;
			}

			try {
				auto pool_size = n_inserts * n_threads *
						sizeof(int) * 65 +
					20 * PMEMOBJ_MIN_POOL;

				pop = pmem::obj::pool<root>::create(
					path, LAYOUT, pool_size,
					CREATE_MODE_RW);
				pmem::obj::transaction::run(pop, [&] {
					pop.root()->pptr =
						pmem::obj::make_persistent<
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
				std::cerr << "!pool::open: " << pe.what()
					  << std::endl;
				return 1;
			}
			std::cout << measure<std::chrono::milliseconds>([&] {
				open(pop);
			}) << "ms" << std::endl;
		}

		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "!pool::close: " << e.what() << std::endl;
		return 1;
	} catch (const std::exception &e) {
		std::cerr << "!exception: " << e.what() << std::endl;
		try {
			pop.close();
		} catch (const std::logic_error &e) {
			std::cerr << "!exception: " << e.what() << std::endl;
		}
		return 1;
	}
	return 0;
}
