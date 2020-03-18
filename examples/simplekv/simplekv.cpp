// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * simplekv.cpp -- implementation of simple kv which uses vector to hold
 * values, string as a key and array to hold buckets
 */

#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/pool.hpp>
#include <stdexcept>

#include "simplekv.hpp"

using kv_type = simple_kv<int, 10>;

struct root {
	pmem::obj::persistent_ptr<kv_type> kv;
};

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0]
		  << " file-name [get key|put key value]" << std::endl;
}

int
main(int argc, char *argv[])
{
	if (argc < 3) {
		show_usage(argv);
		return 1;
	}

	const char *path = argv[1];

	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::open(path, "simplekv");
		auto r = pop.root();

		if (r->kv == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				r->kv = pmem::obj::make_persistent<kv_type>();
			});
		}

		if (std::string(argv[2]) == "get" && argc == 4)
			std::cout << r->kv->get(argv[3]) << std::endl;
		else if (std::string(argv[2]) == "put" && argc == 5)
			r->kv->put(argv[3], std::stoi(argv[4]));
		else {
			show_usage(argv);

			pop.close();

			return 1;
		}
	} catch (pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr
			<< "To create pool run: pmempool create obj --layout=simplekv -s 100M path_to_pool"
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
