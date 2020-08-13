// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * get.cpp -- this simple benchmark is used to measure time of getting and
 * changing a specified number of elements from a persistent array using
 * self_relative_ptr and persistent_ptr
 */

#include <cassert>
#include <iostream>

#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include "../measure.hpp"

#ifndef _WIN32

#include <unistd.h>
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

#else

#include <windows.h>
#define CREATE_MODE_RW (S_IWRITE | S_IREAD)

#endif

static const std::string LAYOUT = "get";

using value_type = int[];

constexpr int ARR_SIZE = 10000000;

struct root {
	pmem::obj::persistent_ptr<value_type> pptr;
};

pmem::obj::persistent_ptr<value_type>
prepare_array(pmem::obj::pool_base &pop, int arr_size)
{
	pmem::obj::persistent_ptr<value_type> parr;

	pmem::obj::transaction::run(pop, [&] {
		parr = pmem::obj::make_persistent<value_type>(
			static_cast<std::size_t>(arr_size));
	});

	for (int i = 0; i < arr_size; ++i) {
		parr[i] = i;
	}

	return parr;
}

int
main(int argc, char *argv[])
{
	using pool = pmem::obj::pool<root>;
	pool pop;

	if (argc != 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	const char *path = argv[1];

	try {
		try {
			pop = pool::create(path, LAYOUT, PMEMOBJ_MIN_POOL * 20,
					   CREATE_MODE_RW);
		} catch (const pmem::pool_error &pe) {
			pop = pool::open(path, LAYOUT);
		}

		auto root = pop.root();
		root->pptr = prepare_array(pop, ARR_SIZE);

		pmem::obj::persistent_ptr<value_type> pptr = root->pptr;
		pmem::obj::experimental::self_relative_ptr<value_type>
			offset_ptr = root->pptr;
		int *vptr = root->pptr.get();

		std::cout << "Run time volatile ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     for (int i = 0; i < ARR_SIZE; i++) {
					     vptr[i] += 1;
				     }
			     })
			  << "ms" << std::endl;

		std::cout << "Run time self-relative ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     for (int i = 0; i < ARR_SIZE; i++) {
					     offset_ptr[i] += 1;
				     }
			     })
			  << "ms" << std::endl;

		std::cout << "Run time persistent ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     for (int i = 0; i < ARR_SIZE; i++) {
					     pptr[i] += 1;
				     }
			     })
			  << "ms" << std::endl;

		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<value_type>(
				pop.root()->pptr, ARR_SIZE);
		});

		pop.close();
	} catch (const pmem::pool_error &pe) {
		std::cerr << "!pool::create: " << pe.what() << " " << path
			  << std::endl;
		return 1;
	} catch (const std::logic_error &e) {
		std::cerr << "!pool::close: " << e.what() << std::endl;
		return 1;
	} catch (const std::exception &e) {
		std::cerr << "!exception: " << e.what() << std::endl;
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<value_type>(
				pop.root()->pptr, ARR_SIZE);
		});
		try {
			pop.close();
		} catch (const std::logic_error &e) {
			std::cerr << "!pool::close: " << e.what() << std::endl;
		}
		return 1;
	}
	return 0;
}
