// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

/*
 * segment_vector.cpp -- C++ documentation snippets.
 */

//! [segment_vector_example]
#include <iostream>
#include <libpmemobj++/container/segment_vector.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

const int N_ELEMENTS = 4096;
const int SEGMENT_SIZE = 1024;

using namespace pmem::obj;
using value_t = p<int>;
using segment_t = pmem::obj::vector<value_t>;

/*
 * exponential_size_array_policy<> is equivalent to:
 * exponential_size_array_policy<pmem::obj::vector>
 */
using seg_vec_exp_arr =
	segment_vector<value_t, exponential_size_array_policy<>>;

/*
 * exponential_size_vector_policy<> is equivalent to:
 * exponential_size_vector_policy<pmem::obj::vector>
 */
using seg_vec_exp_vec =
	segment_vector<value_t, exponential_size_vector_policy<>>;

/*
 * fixed_size_vector_policy<SEGMENT_SIZE> is equivalent to:
 * fixed_size_vector_policy<SEGMENT_SIZE, pmem::obj::vector>
 */
using seg_vec_fix_vec =
	segment_vector<value_t, fixed_size_vector_policy<SEGMENT_SIZE>>;

struct root {
	persistent_ptr<segment_t> pptr0;
	persistent_ptr<seg_vec_exp_arr> pptr1;
	persistent_ptr<seg_vec_exp_vec> pptr2;
	persistent_ptr<seg_vec_fix_vec> pptr3;
};

/* Before running this example, run:
 * pmempool create obj --layout="segment_vector_example" path_to_pool
 */
int
main(int argc, char *argv[])
{
	if (argc != 2)
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;

	auto path = argv[1];
	pool<root> pop;
	persistent_ptr<root> r;

	try {
		pop = pool<root>::open(path, "segment_vector_example");
		r = pop.root();
	} catch (const std::exception &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}

	try {
		if (r->pptr0 == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				r->pptr0 = make_persistent<segment_t>();
				r->pptr1 = make_persistent<seg_vec_exp_arr>();
				r->pptr2 = make_persistent<seg_vec_exp_vec>();
				r->pptr3 = make_persistent<seg_vec_fix_vec>();
			});
		}

		auto &seg_vec_0 = *pop.root()->pptr0;
		auto &seg_vec_1 = *pop.root()->pptr1;
		auto &seg_vec_2 = *pop.root()->pptr2;
		auto &seg_vec_3 = *pop.root()->pptr3;

		for (int i = 0; i < N_ELEMENTS; ++i) {
			seg_vec_0[i] = i;
		}

		seg_vec_1 =
			seg_vec_exp_arr(seg_vec_0.cbegin(), seg_vec_0.cend());
		seg_vec_2 =
			seg_vec_exp_vec(seg_vec_1.cbegin(), seg_vec_1.cend());
		seg_vec_3 =
			seg_vec_fix_vec(seg_vec_2.cbegin(), seg_vec_2.cend());

		for (int i = 0; i < N_ELEMENTS; ++i) {
			assert(seg_vec_1[i] = seg_vec_0[i]);
			assert(seg_vec_2[i] = seg_vec_1[i]);
			assert(seg_vec_3[i] = seg_vec_2[i]);
		}

		seg_vec_0.clear();
		seg_vec_1.clear();
		seg_vec_2.clear();
		seg_vec_3.clear();
	} catch (const pmem::manual_tx_abort &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const pmem::pool_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const pmem::transaction_out_of_memory &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const pmem::transaction_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const std::out_of_range &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const std::logic_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			delete_persistent<segment_t>(r->pptr0);
			delete_persistent<seg_vec_exp_arr>(r->pptr1);
			delete_persistent<seg_vec_exp_vec>(r->pptr2);
			delete_persistent<seg_vec_fix_vec>(r->pptr3);
		});
	} catch (const pmem::transaction_scope_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const pmem::transaction_free_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const pmem::manual_tx_abort &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const pmem::transaction_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
//! [segment_vector_example]
