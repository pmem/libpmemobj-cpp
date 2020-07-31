// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/radix_tree.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>

#include <iostream>

using kv_type = pmem::obj::experimental::radix_tree<
	pmem::obj::experimental::inline_string,
	pmem::obj::experimental::inline_string>;

struct root {
	pmem::obj::persistent_ptr<kv_type> kv;
};

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
}

/* This example reads a sequence of keys from stdin, add them to a radix
 * tree and then print a radix tree representation (in a DOT format).
 *
 * Give a file with some keys, for example input.txt with such content:
 * key1
 * key2
 * key3
 *
 * Run:
 * pmempool create obj --layout=radix -s 100M path_to_pool
 * example-radix_tree_dot path_to_pool < input.txt > graph.txt
 * dot -Tpng graph.txt -o out.png # To generate a png from a radix
 * representation
 */
int
main(int argc, char *argv[])
{
	if (argc < 2) {
		show_usage(argv);
		return 1;
	}

	const char *path = argv[1];

	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::open(path, "radix");
		auto r = pop.root();

		if (r->kv == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				r->kv = pmem::obj::make_persistent<kv_type>();
			});
		}

		std::string k;

		while (std::cin >> k) {
			r->kv->try_emplace(k, k);
		}

		std::cout << *(r->kv) << std::endl;
	} catch (pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr
			<< "To create pool run: pmempool create obj --layout=radix -s 100M path_to_pool"
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
