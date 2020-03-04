// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

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
const bool remove_hashmap = true;

// This is basic example and we only need to use concurrent_hash_map. Hence we
// will correlate memory pool root object with single instance of persistent
// pointer to hasmap_type
struct root {
	persistent_ptr<hashmap_type> pptr;
};

int
main(int argc, char *argv[])
{
	try {
		if (argc != 2)
			std::cerr << "usage: " << argv[0] << " file-name"
				  << std::endl;

		auto path = argv[1];

		auto pop =
			pool<root>::open(path, "concurrent_hash_map example");
		auto r = pop.root()->pptr;

		if (r == nullptr) {
			// Logic when file didn't exist when open() was called.
			// After the pool was created, we have to allocate
			// object of hashmap_type and attach it to the root
			// object.
			pmem::obj::transaction::run(pop, [&] {
				r = make_persistent<hashmap_type>();
			});
		} else {
			// Logic when file already exists. After opening of the
			// pool we have to call runtime_initialize() function in
			// order to recalculate mask and check for consistentcy.

			r->runtime_initialize();

			// defragment the whole pool at the beginning
			try {
				r->defragment();
			} catch (const pmem::defrag_error &e) {
				std::cerr << "Defragmentation exception: "
					  << e.what() << std::endl;
				pop.close();
				return -1;
			}
		}

		auto &map = *r;

		std::vector<std::thread> threads;
		threads.reserve(static_cast<size_t>(THREADS_NUM));

		// Insert THREADS_NUM / 3 key-value pairs to the hashmap. This
		// operation is thread-safe.
		for (int i = 0; i < THREADS_NUM / 3; ++i) {
			threads.emplace_back([&]() {
				for (int i = 0; i < 10 * THREADS_NUM; ++i) {
					map.insert(
						hashmap_type::value_type(i, i));
				}
			});
		}

		// Erase THREADS_NUM /3 key-value pairs from the hashmap. This
		// operation is thread-safe.
		for (int i = 0; i < THREADS_NUM / 3; ++i) {
			threads.emplace_back([&]() {
				for (int i = 0; i < 10 * THREADS_NUM; ++i) {
					map.erase(i);
				}
			});
		}

		// Check if given key is in the hashmap. For the time of an
		// accessor life, the read-write lock is taken on the item.
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
		try {
			// defragment the whole pool at the end
			map.defragment();
		} catch (const pmem::defrag_error &e) {
			std::cerr << "Defragmentation exception: " << e.what()
				  << std::endl;
			pop.close();
			return -1;
		} catch (const pmem::lock_error &e) {
			std::cerr << "Defragmentation exception: " << e.what()
				  << std::endl;
			return -1;
		} catch (const std::runtime_error &e) {
			std::cerr << "Defragmentation exception: " << e.what()
				  << std::endl;
			pop.close();
			return -1;
		} catch (const std::range_error &e) {
			std::cerr << "Defragmentation exception: " << e.what()
				  << std::endl;
			return -1;
		}
		// Erase remaining itemes in map. This function is not
		// thread-safe, hence the function is being called only after
		// thread execution has completed.
		try {
			map.clear();
		} catch (const pmem::transaction_out_of_memory &e) {
			std::cerr << "Clear exception: " << e.what()
				  << std::endl;
			return -1;
		} catch (const pmem::transaction_free_error &e) {
			std::cerr << "Clear exception: " << e.what()
				  << std::endl;
			return -1;
		}

		// If hash map is to be removed, free_data() method should be
		// called first. Otherwise, if deallocating internal hash map
		// metadata in a destructor fail program might terminate.
		if (remove_hashmap) {
			map.free_data();

			// map.clear() // WRONG
			// After free_data() concurrent hash map cannot be used
			// anymore!

			transaction::run(pop, [&] {
				delete_persistent<hashmap_type>(r);
			});
		}

		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "Logic error: " << e.what() << std::endl;
		return -1;
	} catch (const pmem::transaction_error &e) {
		std::cerr << "Transaction exception: " << e.what() << std::endl;
		return -1;
	} catch (const pmem::pool_error &e) {
		std::cerr << "Pool exception: " << e.what() << std::endl;
		return -1;
	} catch (const pmem::layout_error &e) {
		std::cerr << "Layout exception: " << e.what() << std::endl;
		return -1;
	} catch (...) {
		std::cerr << "Unexpected exception occured!" << std::endl;
		return -1;
	}
	return 0;
}

//! [concurrent_hash_map_example]
