// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/**
 * radix_tree.cpp -- this simple benchmark is used to compare times of basic
 * operations in radix_tree and std::map
 */

#include <map>
#include <vector>

#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/radix_tree.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>

#include "../measure.hpp"

struct data {
	unsigned long index;
	unsigned long data_1, data_2;
};

using value_type = struct data;

using kv_type = pmem::obj::experimental::radix_tree<unsigned long, value_type>;

struct root {
	pmem::obj::persistent_ptr<kv_type> kv;
};

static std::vector<unsigned long> keys_to_insert;
static std::vector<unsigned long> keys_to_lookup;
static std::vector<unsigned long> ne_keys;

static std::map<unsigned long, value_type> mymap;

/* default size */
static size_t count = 10000;
static size_t sample_size = 100;
static size_t batch_size = 1000;

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0]
		  << " file-name [count] [batch_size] [sample_size]"
		  << std::endl;
}

/* prepare odd keys to check finding non-existing keys (in the containers will
 * be only even keys) */
void
gen_ne_keys(void)
{
	unsigned long key;
	unsigned key1;

	for (size_t i = 0; i < (count / sample_size); ++i) {
		key = static_cast<unsigned long>(rand());
		key <<= 32;
		do {
			key1 = static_cast<unsigned>(rand());
		} while ((key1 & 0x1) == 0);
		key = key | key1;
		ne_keys.emplace_back(key);
	}
}

void
gen_keys()
{
	unsigned long key;
	unsigned key1;

	for (size_t i = 0; i < count; ++i) {
		/* only even keys will be insterted */
		key = static_cast<unsigned long>(rand());
		key <<= 32;
		do {
			key1 = static_cast<unsigned>(rand());
		} while ((key1 & 0x1));
		key = key | key1;

		keys_to_insert.emplace_back(key);

		/* store one element in every SAMPLE, for future lookup */
		if (i % sample_size == 0)
			keys_to_lookup.emplace_back(key);
	}
}

/* insert_f must insert batch_size elements */
template <typename F>
void
insert_elements_kv(F &&insert_f, const std::string &container)
{
	std::chrono::nanoseconds::rep insert_time = 0;

	std::cout << "Inserting " << count << " elements..." << std::endl;

	for (size_t j = 0; j < count; j += batch_size)
		insert_time +=
			measure<std::chrono::nanoseconds>([&] { insert_f(j); });

	std::cout << "Average insert time: (" << container
		  << "): " << static_cast<unsigned long>(insert_time) / count
		  << "ns" << std::endl;
}

void
lookup_elements_kv(pmem::obj::pool<root> pop)
{
	auto r = pop.root();

	std::cout << "Looking up " << keys_to_lookup.size() << " elements..."
		  << std::endl;

	kv_type::iterator res_radix;
	std::map<unsigned long, value_type>::iterator res_map;

	auto radix_time = measure<std::chrono::nanoseconds>([&] {
		for (auto &key : keys_to_lookup)
			res_radix = r->kv->find(key);
	});
	std::cout << "Average access time (persistent radix tree): "
		  << static_cast<unsigned long>(radix_time) /
			keys_to_lookup.size()
		  << "ns" << std::endl;

	auto std_map_time = measure<std::chrono::nanoseconds>([&] {
		for (auto &key : keys_to_lookup)
			res_map = mymap.find(key);
	});
	std::cout << "Average access time (map): "
		  << static_cast<unsigned long>(std_map_time) /
			keys_to_lookup.size()
		  << "ns" << std::endl;
}

void
lookup_ne_elements_kv(pmem::obj::pool<root> pop)
{
	auto r = pop.root();

	std::cout << "[Key not present] Looking up " << ne_keys.size()
		  << " elements..." << std::endl;

	kv_type::iterator res_radix;
	std::map<unsigned long, value_type>::iterator res_map;

	auto radix_time = measure<std::chrono::nanoseconds>([&] {
		for (auto &key : ne_keys)
			res_radix = r->kv->find(key);
	});
	std::cout
		<< "[Key not present] Average access time (persistent radix tree): "
		<< static_cast<unsigned long>(radix_time) / ne_keys.size()
		<< "ns" << std::endl;

	auto std_map_time = measure<std::chrono::nanoseconds>([&] {
		for (auto &key : ne_keys) {
			res_map = mymap.find(key);
		}
	});
	std::cout << "[Key not present] Average access time (map): "
		  << static_cast<unsigned long>(std_map_time) / ne_keys.size()
		  << "ns" << std::endl;
}

void
remove_all_elements_kv(pmem::obj::pool<root> pop)
{
	auto r = pop.root();

	std::cout << "Removing " << keys_to_insert.size() << " elements..."
		  << std::endl;

	auto radix_time = measure<std::chrono::nanoseconds>([&] {
		for (auto it = r->kv->begin(); it != r->kv->end();)
			it = r->kv->erase(it);
	});
	std::cout << "Average remove time (persistent radix tree): "
		  << static_cast<unsigned long>(radix_time) /
			keys_to_insert.size()
		  << "ns" << std::endl;

	auto std_map_time = measure<std::chrono::nanoseconds>([&] {
		for (auto it = mymap.begin(); it != mymap.end();) {
			auto tmp_it = it++;
			mymap.erase(tmp_it);
		}
	});
	std::cout << "Average remove time (map): "
		  << static_cast<unsigned long>(std_map_time) /
			keys_to_insert.size()
		  << "ns" << std::endl;
}

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		show_usage(argv);
		return 1;
	}

	const char *path = argv[1];
	if (argc > 2)
		count = std::stoul(argv[2]);
	if (argc > 3)
		batch_size = std::stoul(argv[3]);
	if (argc > 4)
		sample_size = std::stoul(argv[4]);

	std::cout << "Radix benchmark, count: " << count
		  << ", batch_size: " << batch_size
		  << ", sample_size: " << sample_size << std::endl;

	srand(time(0));

	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::open(path, "radix");
		auto r = pop.root();

		if (r->kv == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				r->kv = pmem::obj::make_persistent<kv_type>();
			});
		}
	} catch (pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr
			<< "To create pool run: pmempool create obj --layout=radix -s 1G path_to_pool"
			<< std::endl;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	gen_ne_keys();
	gen_keys();
	try {
		insert_elements_kv(
			[&](size_t start) {
				auto r = pop.root();
				pmem::obj::transaction::run(pop, [&] {
					for (size_t i = start;
					     i < start + batch_size; ++i) {
						value_type value;
						value.index = i;

						r->kv->try_emplace(
							keys_to_insert[i],
							value);
					}
				});
			},
			"persistent radix tree");

		insert_elements_kv(
			[&](size_t start) {
				for (size_t i = start; i < start + batch_size;
				     ++i) {
					value_type map_value;
					map_value.index = i;
					mymap.emplace(keys_to_insert[i],
						      map_value);
				}
			},
			"map");

		lookup_elements_kv(pop);

		lookup_ne_elements_kv(pop);

		remove_all_elements_kv(pop);

		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
