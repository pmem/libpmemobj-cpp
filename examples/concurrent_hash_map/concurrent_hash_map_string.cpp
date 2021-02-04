// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * concurrent_hash_map_string.cpp -- example shows how to store strings in
 * pmem::obj::concurrent_hash_map
 */

//! [cmap_string_ex]
#include <iostream>
#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <string>
#include <vector>

/* In this example we use concurrent_hash_map with p<int> type as
 * keys and pmem::obj::string type for values. It shows, there is no need to
 * explicitly use transacions if strings are stored in concurrent_hash_map. */

using hashmap_type =
	pmem::obj::concurrent_hash_map<pmem::obj::p<int>, pmem::obj::string>;

const int THREADS_NUM = 30;

/* In this example we need to place concurrent_hash_map in the pool. Hence
 * correlate memory pool root object with a single instance of persistent
 * pointer to hashmap_type */
struct root {
	pmem::obj::persistent_ptr<hashmap_type> pptr;
};

int
main(int argc, char *argv[])
{
	pmem::obj::pool<root> pop;
	bool remove_hashmap = false;
	int retval = 0;

	try {
		if (argc < 2) {
			std::cerr
				<< "usage: " << argv[0]
				<< " file-name [remove_hashmap]" << std::endl
				<< "Before running this example, run:"
				<< std::endl
				<< "pmempool create obj --layout=\"cmap_string\" --size 1G path_to_a_pool"
				<< std::endl;
			return -1;
		}

		auto path = argv[1];

		if (argc == 3)
			remove_hashmap = std::string(argv[2]) == "1";

		try {
			pop = pmem::obj::pool<root>::open(path, "cmap_string");
		} catch (pmem::pool_error &e) {
			std::cerr << e.what() << std::endl;
			return -1;
		}

		auto &r = pop.root()->pptr;

		if (r == nullptr) {
			/* Logic when file was first opened. First, we have to
			 * allocate object of hashmap_type and attach it to the
			 * root object. */
			pmem::obj::transaction::run(pop, [&] {
				r = pmem::obj::make_persistent<hashmap_type>();
			});

			r->runtime_initialize();
		} else {
			/* Logic when hash_map already exists. After opening of
			 * the pool we have to call runtime_initialize()
			 * function in order to recalculate mask and check for
			 * consistency. */
			r->runtime_initialize();

			/* Defragment the whole pool at the beginning */
			try {
				r->defragment();
			} catch (const pmem::defrag_error &e) {
				std::cerr << "Defragmentation exception: "
					  << e.what() << std::endl;
				throw;
			}
		}

		auto &map = *r;
		std::cout << " Number of elements at application startup: "
			  << map.size() << std::endl;

		std::vector<std::thread> threads;
		threads.reserve(static_cast<size_t>(THREADS_NUM));

		/* Start THREADS_NUM/3 threads to insert key-value pairs
		 * to the hashmap. This operation is thread-safe. */
		for (int j = 0; j < THREADS_NUM / 3; ++j) {
			threads.emplace_back([&]() {
				for (int i = 0; i < 10 * THREADS_NUM; ++i) {
					/* Implicit conversion from std::string
					 * to pmem::obj::string. */
					map.insert_or_assign(i,
							     std::to_string(i));
				}
			});
		}

		/* Start THREADS_NUM/3 threads to check if given key is
		 * in the hashmap. For the time of an accessor life,
		 * the read-write lock is taken on the item. */
		for (int j = 0; j < THREADS_NUM / 3; ++j) {
			threads.emplace_back([&]() {
				for (int i = 0; i < 10 * THREADS_NUM; ++i) {
					/* Usage of const_accessor, indicates
					 * read-only access */
					hashmap_type::const_accessor acc;
					bool res = map.find(acc, i);

					if (res) {
						assert(acc->first == i);
						/* Pointer to the value may be
						 * used as long as the accessor
						 * object exists. */
						const pmem::obj::string
							*element = &acc->second;
						std::cout << element->c_str()
							  << std::endl;
					}
				}
			});
		}

		/* Start THREADS_NUM/3 threads to erase key-value pairs
		 * from the hashmap. This operation is thread-safe. */
		for (int j = 0; j < THREADS_NUM / 3; ++j) {
			threads.emplace_back([&]() {
				for (int i = 0; i < 10 * THREADS_NUM; ++i) {
					map.erase(i);
				}
			});
		}

		for (auto &t : threads) {
			t.join();
		}
		try {
			/* Defragment the whole pool at the end */
			map.defragment();
		} catch (const pmem::defrag_error &e) {
			std::cerr << "Defragmentation exception: " << e.what()
				  << std::endl;
			throw;
		} catch (const pmem::lock_error &e) {
			std::cerr << "Defragmentation exception: " << e.what()
				  << std::endl;
			throw;
		} catch (const std::range_error &e) {
			std::cerr << "Defragmentation exception: " << e.what()
				  << std::endl;
			throw;
		} catch (const std::runtime_error &e) {
			std::cerr << "Defragmentation exception: " << e.what()
				  << std::endl;
			throw;
		}

		if (remove_hashmap) {
			/* Firstly, erase remaining items in the map. This
			 * function is not thread-safe, hence the function is
			 * being called only after thread execution has
			 * completed. */
			try {
				map.clear();
			} catch (const pmem::transaction_out_of_memory &e) {
				std::cerr << "Clear exception: " << e.what()
					  << std::endl;
				throw;
			} catch (const pmem::transaction_free_error &e) {
				std::cerr << "Clear exception: " << e.what()
					  << std::endl;
				throw;
			}

			/* If hash map is to be removed, free_data() method
			 * should be called first. Otherwise, if deallocating
			 * internal hash map metadata in a destructor fails
			 * program might terminate. */
			map.free_data();

			/* map.clear() // WRONG
			 * After free_data() concurrent hash map cannot be used
			 * anymore! */

			pmem::obj::transaction::run(pop, [&] {
				pmem::obj::delete_persistent<hashmap_type>(r);
				r = nullptr;
			});
		}
	} catch (std::exception &e) {
		std::cerr << "Exception occured: " << e.what() << std::endl;
		retval = -1;
	}
	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		retval = -2;
	}
	return retval;
}
//! [cmap_string_ex]
