// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * get.cpp -- this simple benchmarks is used to measure time of
 * getting specified number of elements and time of runtime_initialize().
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

static const std::string LAYOUT = "assignment";

using value_type = std::size_t;
using size_type = std::size_t;
template <typename U>
using persistent_ptr = pmem::obj::persistent_ptr<U>;
template <typename U>
using self_relative_ptr = pmem::obj::experimental::self_relative_ptr<U>;
template <typename U>
using vector = pmem::obj::vector<U>;

constexpr size_type ARR_SIZE = 1000;
using vector_persist = vector<persistent_ptr<value_type>>;
using vector_relative = vector<self_relative_ptr<value_type>>;

struct root {
	persistent_ptr<vector_persist> vec_pers_ptr;
	persistent_ptr<vector_relative> vec_self_ptr;
};

template <template <typename U> class pointer>
void
prepare_array(pmem::obj::pool_base &pop,
	      self_relative_ptr<vector<pointer<value_type>>> arr_pointers,
	      persistent_ptr<value_type> ptr)
{
	arr_pointers->reserve(ARR_SIZE);

	for (size_type i = 0; i < ARR_SIZE; ++i) {
		arr_pointers->emplace_back(pointer<value_type>{ptr});
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
					   S_IWUSR | S_IRUSR);
		} catch (const pmem::pool_error &pe) {
			std::cerr << "!pool::create: " << pe.what() << " "
				  << path << std::endl;
			pop = pool::open(path, LAYOUT);
		}

		auto root = pop.root();

		persistent_ptr<value_type> ptr;
		persistent_ptr<value_type> ptr2;
		pmem::obj::transaction::run(pop, [&] {
			root->vec_pers_ptr =
				pmem::obj::make_persistent<vector_persist>();
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
					     auto &persist_array =
						     *(root->vec_pers_ptr);
					     auto ptr_copy = ptr;
					     for (size_type i = 0; i < ARR_SIZE;
						  i++) {
						     swap(persist_array[i],
							  ptr_copy);
					     }
				     });
			     })
			  << "ms" << std::endl;

		std::cout << "Run time assignment persistent ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     pmem::obj::transaction::run(pop, [&] {
					     auto &persist_array =
						     *(root->vec_pers_ptr);
					     for (size_type i = 0; i < ARR_SIZE;
						  i++) {
						     persist_array[i] = ptr2;
					     }
				     });
			     })
			  << "ms" << std::endl;

		pmem::obj::transaction::run(pop, [&] {
			root->vec_pers_ptr->free_data();
			pmem::obj::delete_persistent<vector_persist>(
				root->vec_pers_ptr);
			root->vec_self_ptr =
				pmem::obj::make_persistent<vector_relative>();
			prepare_array<self_relative_ptr>(
				pop, root->vec_self_ptr, ptr);
		});

		std::cout << "Run time swap self-relative ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     pmem::obj::transaction::run(pop, [&] {
					     auto &self_array =
						     *(root->vec_self_ptr);
					     auto self_ptr_copy = self_ptr;
					     for (size_type i = 0; i < ARR_SIZE;
						  i++) {
						     swap(self_array[i],
							  self_ptr_copy);
					     }
				     });
			     })
			  << "ms" << std::endl;

		std::cout << "Run time assignment self-relative ptr "
			  << measure<std::chrono::milliseconds>([&] {
				     pmem::obj::transaction::run(pop, [&] {
					     auto &self_array =
						     *(root->vec_self_ptr);
					     for (size_type i = 0; i < ARR_SIZE;
						  i++) {
						     self_array[i] = self_ptr2;
					     }
				     });
			     })
			  << "ms" << std::endl;

		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<value_type>(ptr);
			pmem::obj::delete_persistent<value_type>(ptr2);
			root->vec_self_ptr->free_data();
			pmem::obj::delete_persistent<vector_relative>(
				root->vec_self_ptr);
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
