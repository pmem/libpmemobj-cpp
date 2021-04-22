// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */


#include "unittest.hpp"
#include <cstring>
#include <iostream>
#include <cstring>

#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/make_persistent.hpp>


#include <libpmemobj++/container/mpsc_queue.hpp>
#include <string>

#define LAYOUT "layout"

struct root {
	pmem::obj::persistent_ptr<char[]> log;
};


int basic_test(int argc, char *argv[])
{

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<struct root> pop;

	bool already_exists = false;
	try {
		pop = pmem::obj::pool<root>::create(
			std::string(path) , LAYOUT,
			PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		already_exists = true;
		pop = pmem::obj::pool<root>::open(std::string(path) , LAYOUT);
	}

	auto proot = pop.root();

	auto queue = new pmem::obj::experimental::mpsc_queue(&pop, &proot->log, 10000, 1);
	auto worker =  queue->register_worker();
	if(!already_exists){
	{
		auto acc = worker.produce(4);
		const char *tmp  = "asdf";
		acc.add(tmp, 4);
	}
	{
		auto acc = worker.produce(10);
		const char *tmp  = "asdf1";
		acc.add(tmp, 5);
		acc.add("zzzzz", 5);
	}
	{
		auto acc = worker.produce(5);
		const char *tmp  = "asdf1";
		acc.add(tmp, 5);
	}
	}
	{
		auto rd_acc = queue->consume();
		std::cout << std::string(rd_acc.data, rd_acc.len) << std::endl;
	}
	{
		auto acc = worker.produce(5);
		const char *tmp  = "old";
		acc.add(tmp, 3);
	}

	delete queue;
	return 0;
}

int mt_test(int argc, char *argv[])
{
	size_t concurrency = 16;

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<struct root> pop;
	
	bool recovery_needed = false;
	try {
		pop = pmem::obj::pool<root>::create(
			std::string(path) , LAYOUT,
			PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		pop = pmem::obj::pool<root>::open(std::string(path) , LAYOUT);
		recovery_needed = true;
	}

	auto proot = pop.root();
	const size_t buff_size = 100000;
	
	if(! recovery_needed)
	{
		pmem::obj::transaction::run(pop, [&] {
				proot->log = pmem::obj::make_persistent<char[]>(buff_size);
		});
	}
	auto queue = new pmem::obj::experimental::mpsc_queue(&pop, &proot->log, buff_size, concurrency);
	if(recovery_needed)
	{
		queue-> recover();
		auto rd_acc = queue->consume();
		if(rd_acc.len > 0)
			std::cout << std::string(rd_acc.data, rd_acc.len) << std::endl;
	} else {

		parallel_exec(concurrency, [&](size_t thread_id) {
			if( thread_id == 0 ){
				for (int i = 0; i<100000000; i++)
				{
					auto rd_acc = queue->consume();
					if(rd_acc.len > 0)
						std::cout << std::string(rd_acc.data, rd_acc.len) << std::endl;
				}
			} else {
				auto worker =  queue->register_worker();
				for( int i =0; i<10; i++)
				{
					std::string tmp = std::to_string(i);
					auto acc = worker.produce(tmp.size());
					acc.add(tmp.data(), tmp.size());
				}
			}
		});
	}
	delete queue;
	return 0;
}

int
main(int argc, char *argv[])
{

	return run_test([&] {
		mt_test(argc, argv);
	});
}
