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

#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

const static size_t pool_size = PMEMOBJ_MIN_POOL;
const static size_t test_val1 = 123U;

using vector_type = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<vector_type> pptr1;
	nvobj::persistent_ptr<vector_type> pptr2;
};

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path,
					     "VectorTest: vector_ctor_capacity",
					     pool_size, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr1 = nvobj::make_persistent<vector_type>();
			/* test capacity of default-constructed vector */
			UT_ASSERT(0 == r->pptr1->capacity());
			nvobj::delete_persistent<vector_type>(r->pptr1);

			r->pptr1 = nvobj::make_persistent<vector_type>(
				test_val1, 0);
			/* test capacity of size-value-constructed vector */
			UT_ASSERT(test_val1 == r->pptr1->capacity());

			r->pptr2 = nvobj::make_persistent<vector_type>(
				r->pptr1->begin(), r->pptr1->end());
			/* test capacity of iter-iter-constructed vector */
			UT_ASSERT(test_val1 == r->pptr2->capacity());
			nvobj::delete_persistent<vector_type>(r->pptr2);

			r->pptr2 =
				nvobj::make_persistent<vector_type>(*r->pptr1);
			/* test capacity of copy-constructed vector */
			UT_ASSERT(test_val1 == r->pptr2->capacity());
			nvobj::delete_persistent<vector_type>(r->pptr2);

			r->pptr2 = nvobj::make_persistent<vector_type>(
				std::move(*r->pptr1));
			/* test capacity of move-constructed vector */
			UT_ASSERT(test_val1 == r->pptr2->capacity());
			nvobj::delete_persistent<vector_type>(r->pptr2);
			nvobj::delete_persistent<vector_type>(r->pptr1);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
