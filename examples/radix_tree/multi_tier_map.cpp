// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/**
 * multi_tier_map.cpp -- example which shows how to use
 * pmem::obj::experimental::radix_tree with a DRAM caching layer.
 */

#include <libpmemobj++/experimental/radix_tree.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>

#include <atomic>
#include <map>
#include <thread>

#include <libpmemobj++/container/mpsc_queue.hpp>
#include <iostream>
#include <string>

struct data {
	pmem::obj::p<size_t> data_1;
	pmem::obj::p<size_t> data_2;
	pmem::obj::p<size_t> data_3;

	bool
	operator==(const data &rhs) const
	{
		return data_1 == rhs.data_1 && data_2 == rhs.data_2 &&
			data_3 == rhs.data_3;
	}
};

using kv_type = pmem::obj::experimental::radix_tree<size_t, data>;

struct root {
	pmem::obj::persistent_ptr<kv_type> kv;
	pmem::obj::persistent_ptr<char[]> log;
};

//static constexpr size_t LOG_CAPACITY = (1ULL << 30);
static constexpr size_t LOG_CAPACITY = (1ULL << 12);

struct heterogenous_map {
	heterogenous_map(size_t dram_size, kv_type &kv,
			 pmem::obj::persistent_ptr<char[]> *log)
	    : dram_size(dram_size), kv(kv), log_size(10000), log(log), queue(log, log_size, 1)
	{
		stopped.store(false);
		bg_thread = std::thread([&] { bg_work(); });

		pop = pmem::obj::pool_by_vptr(this->log);
	}

	~heterogenous_map()
	{
		stopped.store(true);
		bg_thread.join();
	}

	void
	insert(const size_t &k, const data &v)
	{
		auto req_size = sizeof(log_data);

		// evict random data
		if (map.size() >= dram_size) {
			map.erase(map.begin());
		}

		//auto log_pos = log + log_size;
		map.insert_or_assign(k, v);

		log_data ld;
		ld.key = k;
		ld.value = v;

		auto worker = queue.register_worker();
		auto acc = worker.produce(req_size);
		acc.add((char *)&ld, req_size);
	}

	void
	remove(const size_t &k)
	{
		insert(k, tombstone());
	}

	data
	get(const size_t &k)
	{
		auto it = map.find(k);
		if (it != map.end()) {
			return it->second;
		} else {
			auto it = kv.find(k);
			if (it != kv.end())
				return kv.find(k)->value();
			else
				return tombstone();
		}
	}

private:
	struct log_data {
		size_t key;
		data value;
	};

	size_t dram_size;
	kv_type &kv;
	std::map<size_t, data> map;

//	char *log;
	size_t log_capacity;
	size_t log_size;
	pmem::obj::persistent_ptr<char[]> *log;

	pmem::obj::experimental::mpsc_queue queue;

	std::atomic<bool> stopped;
	std::thread bg_thread;

	pmem::obj::pool_base pop;

	void
	bg_work()
	{
		while (!stopped.load()) {
			auto acc = queue.consume();
			if(acc.len == 0)
				continue;

			log_data *ptr = (log_data *) acc.data;
			// XXX - make sure tx does not abort and use
			// defer_free
			if (ptr->value == tombstone()) {
				kv.erase(ptr->key);
			} else {
				kv.insert_or_assign(ptr->key,
						    ptr->value);
			}
			
		}
	}

	static data
	tombstone()
	{
		return data{std::numeric_limits<size_t>::max(),
			    std::numeric_limits<size_t>::max(),
			    std::numeric_limits<size_t>::max()};
	}
};

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
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
				r->log = pmem::obj::make_persistent<char[]>(LOG_CAPACITY);
			});
		}

		heterogenous_map map(10, *r->kv, &r->log);

		map.insert(1, data{1, 2, 1});
		map.insert(2, data{1, 2, 2});
		map.insert(3, data{1, 2, 3});
		map.insert(4, data{1, 2, 4});
		map.insert(5, data{1, 2, 0xDEADBEEF});
	
		std::cout << std::to_string( map.get(1).data_3 ) << std::endl;
		std::cout << std::to_string( map.get(2).data_3 ) << std::endl;
		std::cout << std::to_string( map.get(3).data_3 ) << std::endl;
		std::cout << std::to_string( map.get(4).data_3 ) << std::endl;
		std::cout << std::to_string( map.get(5).data_3 ) << std::endl;

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
