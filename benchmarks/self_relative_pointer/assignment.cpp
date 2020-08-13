// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * assignment.cpp -- this benchmarks is used to measure time of
 * the assignment operator and the swap function for persistent_ptr and
 * self_relative_ptr
 */

#include <cassert>
#include <iostream>

#include "../measure.hpp"
#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#ifndef _WIN32

#include <unistd.h>
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

#else

#include <windows.h>
#define CREATE_MODE_RW (S_IWRITE | S_IREAD)

#endif

static const std::string LAYOUT = "assignment";

using value_type = std::size_t;
using size_type = std::ptrdiff_t;
template <typename U>
using persistent_ptr = pmem::obj::persistent_ptr<U>;
template <typename U>
using self_relative_ptr = pmem::obj::experimental::self_relative_ptr<U>;
template <typename U>
using vector = pmem::obj::vector<U>;

constexpr size_type ARR_SIZE = 1000;

struct root {
	persistent_ptr<persistent_ptr<value_type>[]> vec_pers_ptr;
	self_relative_ptr<self_relative_ptr<value_type>[]> vec_self_ptr;
};

template <template <typename U> class pointer>
void
prepare_array(pmem::obj::pool_base &pop,
	      pointer<pointer<value_type>[]> &arr_pointers,
	      persistent_ptr<value_type> ptr)
{
	arr_pointers =
		pmem::obj::make_persistent<pointer<value_type>[]>(ARR_SIZE);

	for (size_type i = 0; i < ARR_SIZE; ++i) {
		arr_pointers[i] = pointer<value_type>{ptr};
	}
}

template <template <typename U> class pointer>
void
benchmark_swap(pointer<pointer<value_type>[]> &array, pointer<value_type> value)
{
	for (size_type i = 0; i < ARR_SIZE; i++) {
		swap(array[i], value);
	}
}

template <template <typename U> class pointer>
void
benchmark_assignment(pointer<pointer<value_type>[]> &array,
		     pointer<value_type> value)
{
	for (size_type i = 0; i < ARR_SIZE; i++) {
		array[i] = value;
	}
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
			std::cerr << "!pool::create: " << pe.what() << " "
				  << path << std::endl;
			pop = pool::open(path, LAYOUT);
		}

		auto root = pop.root();

		persistent_ptr<value_type> ptr;
		persistent_ptr<value_type> ptr2;
		pmem::obj::transaction::run(pop, [&] {
			ptr = pmem::obj::make_persistent<value_type>();
			ptr2 = pmem::obj::make_persistent<value_type>();
			prepare_array<persistent_ptr>(pop, root->vec_pers_ptr,
						      ptr);
		});
		self_relative_ptr<value_type> self_ptr2{ptr2};
		self_relative_ptr<value_type> self_ptr{ptr};

		std::cout << "Run time swap persistent ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     pmem::obj::transaction::run(pop, [&] {
					     benchmark_swap(root->vec_pers_ptr,
							    ptr);
				     });
			     })
			  << "ms" << std::endl;

		std::cout << "Run time assignment persistent ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     pmem::obj::transaction::run(pop, [&] {
					     benchmark_assignment(
						     root->vec_pers_ptr, ptr2);
				     });
			     })
			  << "ms" << std::endl;

		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<
				persistent_ptr<value_type>[]>(
				root->vec_pers_ptr, ARR_SIZE);
			prepare_array<self_relative_ptr>(
				pop, root->vec_self_ptr, ptr);
		});

		std::cout << "Run time swap self-relative ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     pmem::obj::transaction::run(pop, [&] {
					     benchmark_swap(root->vec_self_ptr,
							    self_ptr);
				     });
			     })
			  << "ms" << std::endl;

		std::cout << "Run time assignment self-relative ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     pmem::obj::transaction::run(pop, [&] {
					     benchmark_assignment(
						     root->vec_self_ptr,
						     self_ptr2);
				     });
			     })
			  << "ms" << std::endl;

		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<value_type>(ptr);
			pmem::obj::delete_persistent<value_type>(ptr2);
			pmem::obj::delete_persistent<
				self_relative_ptr<value_type>[]>(
				root->vec_self_ptr, ARR_SIZE);
		});
		pop.close();
	} catch (const pmem::pool_error &pe) {
		std::cerr << "!pool::open: " << pe.what() << " " << path
			  << std::endl;
		return 1;
	} catch (const std::logic_error &e) {
		std::cerr << "!pool::close: " << e.what() << std::endl;
		return 1;
	} catch (const std::exception &e) {
		std::cerr << "!exception: " << e.what() << std::endl;
		try {
			pop.close();
		} catch (const std::logic_error &e) {
			std::cerr << "!pool::close: " << e.what() << std::endl;
		}
		return 1;
	}
	return 0;
}
