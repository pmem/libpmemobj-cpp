// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * radix_tree_inline_string.cpp -- example which shows how to use
 * pmem::obj::experimental::radix_tree with inline_string.
 */

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

void
insert_elements_kv(pmem::obj::pool<root> pop)
{
	auto r = pop.root();

	pmem::obj::transaction::run(pop, [&] {
		r->kv->try_emplace("example1", "1");
		r->kv->try_emplace("example2", "2");

		auto it = r->kv->find("example1");

		assert(pmem::obj::string_view(it->value()).compare("1") == 0);

		++it;
		assert(pmem::obj::string_view(it->value()).compare("2") == 0);

		/* This is OK, inline_string has enough capacity */
		assert(it->value().capacity() >= 1);
		it->value() = "3";

		try {
			it->value() = "some long string";
		} catch (std::out_of_range &) {
			/* The capacity might not be enough, use assign_val
			 * instead. */
		}

		auto it2 = it;
		it.assign_val("some long string");

		/* it is still valid */
		assert(pmem::obj::string_view(it->value())
			       .compare("some long string") == 0);

		/* If reallocation happend on assign_val, it2 will be
		 * invalidated */
		(void)it2;

		/* assert(it2->value().data()[0] == 's'); dangerous, it2 might
		 * be invalid. */
	});
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
		pop = pmem::obj::pool<root>::open(path, "radix");
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
