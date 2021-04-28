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
	size_t index;
	size_t data_1, data_2;
};

struct {
	size_t count = 10000;
	size_t sample_size = 100;
	size_t batch_size = 1000;
} params;

using value_type = struct data;

using kv_type = pmem::obj::experimental::radix_tree<size_t, value_type>;

struct root {
	pmem::obj::persistent_ptr<kv_type> kv;
};

static std::map<size_t, value_type> mymap;

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0]
		  << " file-name [count] [batch_size] [sample_size]"
		  << std::endl;
}

void
print_time_per_element(std::string msg,
		       std::chrono::nanoseconds::rep total_time,
		       size_t n_elements)
{
	std::cout << msg << static_cast<size_t>(total_time) / n_elements << "ns"
		  << std::endl;
}

/* prepare odd keys to check finding non-existing keys (in the containers will
 * be only even keys) */
std::vector<size_t>
gen_nonexisting_keys(size_t count, size_t sample_size)
{
	size_t key;
	size_t key1;

	std::vector<size_t> ret;
	ret.reserve(count / sample_size);
	for (size_t i = 0; i < (count / sample_size); ++i) {
		key = static_cast<size_t>(rand());
		key <<= 32;
		key1 = static_cast<size_t>(rand());
		key |= (key1 | 0x1);
		ret.emplace_back(key);
	}
	return ret;
}

std::vector<size_t>
gen_keys(size_t count)
{
	size_t key;
	size_t key1;

	std::vector<size_t> ret;
	ret.reserve(count);
	for (size_t i = 0; i < count; ++i) {
		/* only even keys will be insterted */
		key = static_cast<size_t>(rand());
		key <<= 32;
		key1 = static_cast<size_t>(rand());
		key |= (key1 & 0x0);
		ret.emplace_back(key);
	}
	return ret;
}

/* insert_f must insert batch_size elements */
template <typename F>
void
insert_elements_kv(F &&insert_f, const std::string &container)
{
	std::chrono::nanoseconds::rep insert_time = 0;

	std::cout << "Inserting " << params.count << " elements..."
		  << std::endl;

	for (size_t i = 0; i < params.count; i += params.batch_size) {
		insert_time +=
			measure<std::chrono::nanoseconds>([&] { insert_f(i); });
	}
	print_time_per_element("Average insert time: (" + container + "): ",
			       insert_time, params.count);
}

void
lookup_elements_kv(pmem::obj::pool<root> pop, std::vector<size_t> &keys)
{
	auto r = pop.root();

	std::cout << "Looking up " << params.count / params.sample_size
		  << " elements..." << std::endl;

	kv_type::iterator res_radix;
	std::map<size_t, value_type>::iterator res_map;

	auto radix_time = measure<std::chrono::nanoseconds>([&] {
		for (size_t i = 0; i < params.count; i += params.sample_size)
			res_radix = r->kv->find(keys[i]);
	});
	print_time_per_element("Average access time (persistent radix tree): ",
			       radix_time, params.count / params.sample_size);

	auto std_map_time = measure<std::chrono::nanoseconds>([&] {
		for (size_t i = 0; i < params.count; i += params.sample_size)
			res_map = mymap.find(keys[i]);
	});
	print_time_per_element("Average access time (map): ", std_map_time,
			       params.count / params.sample_size);
}

void
lookup_ne_elements_kv(pmem::obj::pool<root> pop,
		      std::vector<size_t> &non_existing_keys)
{
	auto r = pop.root();

	std::cout << "[Key not present] Looking up " << non_existing_keys.size()
		  << " elements..." << std::endl;

	kv_type::iterator res_radix;
	std::map<size_t, value_type>::iterator res_map;

	auto radix_time = measure<std::chrono::nanoseconds>([&] {
		for (auto &key : non_existing_keys)
			res_radix = r->kv->find(key);
	});
	print_time_per_element(
		"[Key not present] Average access time (persistent radix tree): ",
		radix_time, non_existing_keys.size());

	auto std_map_time = measure<std::chrono::nanoseconds>([&] {
		for (auto &key : non_existing_keys) {
			res_map = mymap.find(key);
		}
	});
	print_time_per_element("[Key not present] Average access time (map): ",
			       std_map_time, non_existing_keys.size());
}

void
remove_all_elements_kv(pmem::obj::pool<root> pop, std::vector<size_t> &keys)
{
	auto r = pop.root();

	std::cout << "Removing " << keys.size() << " elements..." << std::endl;

	auto radix_time = measure<std::chrono::nanoseconds>([&] {
		for (auto it = r->kv->begin(); it != r->kv->end();)
			it = r->kv->erase(it);
	});
	print_time_per_element("Average remove time (persistent radix tree): ",
			       radix_time, keys.size());

	auto std_map_time = measure<std::chrono::nanoseconds>([&] {
		for (auto it = mymap.begin(); it != mymap.end();) {
			auto tmp_it = it++;
			mymap.erase(tmp_it);
		}
	});
	print_time_per_element("Average remove time (map): ", std_map_time,
			       keys.size());
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
		params.count = std::stoul(argv[2]);
	if (argc > 3)
		params.batch_size = std::stoul(argv[3]);
	if (argc > 4)
		params.sample_size = std::stoul(argv[4]);

	std::cout << "Radix benchmark, count: " << params.count
		  << ", batch_size: " << params.batch_size
		  << ", sample_size: " << params.sample_size << std::endl;

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
		std::cerr
			<< e.what() << std::endl
			<< "To create pool run: pmempool create obj --layout=radix -s 1G path_to_pool"
			<< std::endl;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	auto non_existing_keys =
		gen_nonexisting_keys(params.count, params.sample_size);
	auto keys_to_insert = gen_keys(params.count);
	try {
		insert_elements_kv(
			[&](size_t start) {
				auto r = pop.root();
				pmem::obj::transaction::run(pop, [&] {
					for (size_t i = start;
					     i < start + params.batch_size;
					     ++i) {
						value_type value{i, i + 1,
								 i + 2};

						r->kv->try_emplace(
							keys_to_insert[i],
							value);
					}
				});
			},
			"persistent radix tree");

		insert_elements_kv(
			[&](size_t start) {
				for (size_t i = start;
				     i < start + params.batch_size; ++i) {
					value_type map_value{i, i + 1, i + 2};

					mymap.emplace(keys_to_insert[i],
						      map_value);
				}
			},
			"map");

		lookup_elements_kv(pop, keys_to_insert);
		lookup_ne_elements_kv(pop, non_existing_keys);
		remove_all_elements_kv(pop, keys_to_insert);

		pop.close();
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	} catch (...) {
		std::cerr << "Unexpected exception" << std::endl;
	}
	return 0;
}
