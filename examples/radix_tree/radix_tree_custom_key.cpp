// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * radix_tree_custom_key.cpp -- example which shows how to use
 * pmem::obj::experimental::radix_tree with custom key and custom
 * BytesView.
 */

#include <libpmemobj++/experimental/radix_tree.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>

#include <iostream>

//! [bytes_view_example]
struct custom_key {
	int x;
	int y;
};

struct custom_bytes_view {
	custom_bytes_view(const custom_key *k)
	{
		auto *x = reinterpret_cast<const char *>(&k->x);
		auto *y = reinterpret_cast<const char *>(&k->y);

		for (int i = 0; i < 4; i++)
			bytes[i] = x[sizeof(int) - i - 1];

		for (int i = 0; i < 4; i++)
			bytes[i + 4] = y[sizeof(int) - i - 1];
	}

	/* Note that this function MUST be marked as const */
	char operator[](size_t pos) const
	{
		return bytes[pos];
	}

	/* Note that this function MUST be marked as const */
	size_t
	size() const
	{
		return sizeof(custom_key);
	}

	char bytes[sizeof(custom_key)];
};
//! [bytes_view_example]

/* Alternative implementation of custom_bytes_view could look like this: */
struct alternative_custom_bytes_view {
	alternative_custom_bytes_view(const custom_key *k) : k(k)
	{
	}

	char operator[](size_t pos) const
	{
		auto *x = reinterpret_cast<const char *>(&k->x);
		auto *y = reinterpret_cast<const char *>(&k->y);

		if (pos < 4)
			return x[sizeof(int) - pos - 1];
		else
			return y[sizeof(int) - (pos - 4) - 1];
	}

	size_t
	size() const
	{
		return sizeof(custom_key);
	}

	const custom_key *k;
};

using custom_kv_type =
	pmem::obj::experimental::radix_tree<custom_key, pmem::obj::p<unsigned>,
					    custom_bytes_view>;

struct root {
	pmem::obj::persistent_ptr<custom_kv_type> custom_kv;
};

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
}

void
insert_elements_custom_kv(pmem::obj::pool<root> pop)
{
	auto r = pop.root();

	pmem::obj::transaction::run(pop, [&] {
		r->custom_kv->try_emplace({1, 2}, 1);
		r->custom_kv->try_emplace({3, 4}, 2);

		auto it = r->custom_kv->find({1, 2});

		assert(it->value() == 1);

		++it;
		assert(it->value() == 2);
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

		if (r->custom_kv == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				r->custom_kv = pmem::obj::make_persistent<
					custom_kv_type>();
			});
		}

		insert_elements_custom_kv(pop);

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
