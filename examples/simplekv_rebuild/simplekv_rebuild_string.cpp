// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * simplekv_rebuild_string.cpp -- example usage of simple kv which uses vector
 * to hold values, string as a key and array to hold buckets. This version
 * stores only vectors of keys and values on persistent memory and rebuilds
 * volatile hashmap on restart.
 *
 * This example expects user input from stdin.
 */

#include "simplekv_rebuild.hpp"

#include <stdexcept>
#include <string>

#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/pool.hpp>

using pmem_kv_type = simple_kv_persistent<pmem::obj::string, 10>;
using runtime_kv_type = simple_kv_runtime<pmem::obj::string, 10>;

struct root {
	pmem::obj::persistent_ptr<pmem_kv_type> kv;
};

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	const char *path = argv[1];
	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::open(path,
						  "simplekv_rebuild_string");
		auto r = pop.root();

		if (r->kv == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				r->kv = pmem::obj::make_persistent<
					pmem_kv_type>();
			});
		}

		auto runtime_kv = runtime_kv_type(r->kv.get());

		std::cout << "usage: [get key|put key value|exit]" << std::endl;

		std::string op;
		while (std::cin >> op) {
			std::string key;
			std::string value;

			if (op == "get" && std::cin >> key)
				std::cout << runtime_kv.get(key).data()
					  << std::endl;
			else if (op == "put" && std::cin >> key &&
				 std::cin >> value)
				runtime_kv.put(key, value);
			else if (op == "exit")
				break;
			else {
				std::cout
					<< "usage: [get key|put key value|exit]"
					<< std::endl;
				continue;
			}
		}

	} catch (pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr
			<< "To create pool run: pmempool create obj --layout=simplekv_rebuild_string -s 100M path_to_pool"
			<< std::endl;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
