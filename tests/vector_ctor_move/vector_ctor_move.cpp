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

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

using vector_type = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v1;
	nvobj::persistent_ptr<vector_type> v2;
};

/**
 * Test pmem::obj::experimental::vector move constructor.
 *
 * Checks if vector state is reverted when transaction aborts
 */
void
test_move_ctor_abort(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	auto size = r->v1->size();

	UT_ASSERT(r->v2 == nullptr);
	try {
		nvobj::transaction::run(pop, [&] {
			r->v2 = nvobj::make_persistent<vector_type>(
				std::move(*r->v1));

			UT_ASSERT(r->v1->empty());
			UT_ASSERT(r->v2->size() == size);

			for (vector_type::size_type i = 0; i < size; ++i) {
				UT_ASSERT((*r->v2)[i] == (int)i);
			}

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v2 == nullptr);
	UT_ASSERT(r->v1->size() == size);

	try {
		nvobj::transaction::run(pop, [&] {
			for (vector_type::size_type i = 0; i < size; ++i) {
				UT_ASSERT((*r->v1)[i] == (int)i);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
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
		nvobj::pool<root>::create(path, "VectorTest: vector_ctor_move",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	int arr[] = {0, 1, 2, 3, 4, 5};

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<vector_type>(
				std::begin(arr), std::end(arr));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	test_move_ctor_abort(pop);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<vector_type>(r->v1);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
