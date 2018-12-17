/*
 * Copyright 2018, Intel Corporation
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

#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using vector_type = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v1_pptr;
	nvobj::persistent_ptr<vector_type> v2_pptr;
	nvobj::persistent_ptr<vector_type> v3_pptr;
};

/**
 * Test pmem::obj::experimental::vector comparison operators.
 *
 * Compares elements in two vector containers using following operators:
 * ==, !=, <, <=, >, >=.
 */
void
test_comp_operators(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	int arr1[] = {0, 1, 2, 3, 4};
	int arr2[] = {0, 1, 2, 3, 4, 5};

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1_pptr = nvobj::make_persistent<vector_type>(
				std::begin(arr1), std::end(arr1));
			r->v2_pptr = nvobj::make_persistent<vector_type>(
				std::begin(arr2), std::end(arr2));
			r->v3_pptr = nvobj::make_persistent<vector_type>(
				std::begin(arr2) + 1, std::end(arr2));
		});
	} catch (std::exception &e) {
		UT_ASSERTexc(0, e);
	}

	UT_ASSERT(*(r->v1_pptr) == *(r->v1_pptr));
	UT_ASSERT(*(r->v1_pptr) != *(r->v2_pptr));
	UT_ASSERT(*(r->v1_pptr) != *(r->v3_pptr));
	UT_ASSERT(*(r->v1_pptr) < *(r->v2_pptr));
	UT_ASSERT(*(r->v1_pptr) <= *(r->v2_pptr));
	UT_ASSERT(*(r->v1_pptr) <= *(r->v1_pptr));
	UT_ASSERT(*(r->v2_pptr) > *(r->v1_pptr));
	UT_ASSERT(*(r->v2_pptr) >= *(r->v1_pptr));
	UT_ASSERT(*(r->v2_pptr) >= *(r->v2_pptr));
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: comp_operators",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_comp_operators(pop);

	pop.close();

	return 0;
}
