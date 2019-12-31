/*
 * Copyright 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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

int
main(int argc, char *argv[])
{
	if (argc != 2)
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;

	auto path = argv[1];
	auto pop = pool<root>::open(path, "segment_vector example");
	auto r = pop.root();

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

	seg_vec_1 = seg_vec_exp_arr(seg_vec_0.cbegin(), seg_vec_0.cend());
	seg_vec_2 = seg_vec_exp_vec(seg_vec_1.cbegin(), seg_vec_1.cend());
	seg_vec_3 = seg_vec_fix_vec(seg_vec_2.cbegin(), seg_vec_2.cend());

	for (int i = 0; i < N_ELEMENTS; ++i) {
		assert(seg_vec_1[i] = seg_vec_0[i]);
		assert(seg_vec_2[i] = seg_vec_1[i]);
		assert(seg_vec_3[i] = seg_vec_2[i]);
	}

	seg_vec_0.clear();
	seg_vec_1.clear();
	seg_vec_2.clear();
	seg_vec_3.clear();

	delete_persistent<segment_t>(r->pptr0);
	delete_persistent<seg_vec_exp_arr>(r->pptr1);
	delete_persistent<seg_vec_exp_vec>(r->pptr2);
	delete_persistent<seg_vec_fix_vec>(r->pptr3);

	pop.close();

	return 0;
}

//! [segment_vector_example]
