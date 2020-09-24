// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * radix_tree_inline_string_uint8t_key.cpp -- example which shows how to use
 * pmem::obj::experimental::radix_tree with inline_string<CharT> as a key.
 */

#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/radix_tree.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/string_view.hpp>

#include <iostream>
#include <string>

using CharT = uint8_t;

using kv_type = pmem::obj::experimental::radix_tree<
	pmem::obj::experimental::basic_inline_string<CharT>,
	pmem::obj::p<unsigned>>;

struct root {
	pmem::obj::persistent_ptr<kv_type> kv;
};

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
}

void
insert_elements_kv(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	auto key1 = std::basic_string<CharT>(2, CharT{123});

	const CharT data[] = {123, 123, 123, 0};
	auto key2 = pmem::obj::basic_string_view<CharT>(data);

	r->kv->try_emplace(key1, 1U);
	r->kv->try_emplace(key2, 2U);

	auto it = r->kv->find(key1);

	assert(key1.compare(it->key().data()) == 0);
	assert(it->value() == 1U);

	++it;
	assert(key2.compare(it->key().data()) == 0);
	assert(it->value() == 2U);
}

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
		pop = pmem::obj::pool<root>::open(path, "radix_u8t");
		auto r = pop.root();

		if (r->kv == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				r->kv = pmem::obj::make_persistent<kv_type>();
			});
		}

		insert_elements_kv(pop);

	} catch (pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr
			<< "To create pool run: pmempool create obj --layout=radix_u8t -s 100M path_to_pool"
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
