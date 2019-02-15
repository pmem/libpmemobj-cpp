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

#include "helper_classes.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;

const static size_t pool_size = PMEMOBJ_MIN_POOL;

using test_type = emplace_constructible_copy_insertable_move_insertable<int>;
using vector_type = pmem_exp::vector<test_type>;
using It = test_support::input_it<test_type *>;

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
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: vector_ctor_check_copy", pool_size,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	/**
	 * Check if range ctor will construct vector from element's type
	 * copy ctor.
	 */
	test_type arr[] = {1, 2, 3, 4};

	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr1 = nvobj::make_persistent<vector_type>(
				It(std::begin(arr)), It(std::end(arr)));
		});

		UT_ASSERTeq(r->pptr1->const_at(0).value, 1);
		UT_ASSERTeq(r->pptr1->const_at(1).value, 2);
		UT_ASSERTeq(r->pptr1->const_at(2).value, 3);
		UT_ASSERTeq(r->pptr1->const_at(3).value, 4);

		UT_ASSERTeq(r->pptr1->const_at(0).copied, 1);
		UT_ASSERTeq(r->pptr1->const_at(1).copied, 1);
		UT_ASSERTeq(r->pptr1->const_at(2).copied, 1);
		UT_ASSERTeq(r->pptr1->const_at(3).copied, 1);

		UT_ASSERTeq(r->pptr1->const_at(0).moved, 0);
		UT_ASSERTeq(r->pptr1->const_at(1).moved, 0);
		UT_ASSERTeq(r->pptr1->const_at(2).moved, 0);
		UT_ASSERTeq(r->pptr1->const_at(3).moved, 0);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/**
	 * Check if copy ctor will construct vector from element's type
	 * copy ctor.
	 */
	try {
		nvobj::transaction::run(pop, [&] {
			r->pptr2 =
				nvobj::make_persistent<vector_type>(*r->pptr1);
		});

		UT_ASSERTeq(r->pptr2->const_at(0).value, 1);
		UT_ASSERTeq(r->pptr2->const_at(1).value, 2);
		UT_ASSERTeq(r->pptr2->const_at(2).value, 3);
		UT_ASSERTeq(r->pptr2->const_at(3).value, 4);

		UT_ASSERTeq(r->pptr2->const_at(0).copied, 2);
		UT_ASSERTeq(r->pptr2->const_at(1).copied, 2);
		UT_ASSERTeq(r->pptr2->const_at(2).copied, 2);
		UT_ASSERTeq(r->pptr2->const_at(3).copied, 2);

		UT_ASSERTeq(r->pptr2->const_at(0).moved, 0);
		UT_ASSERTeq(r->pptr2->const_at(1).moved, 0);
		UT_ASSERTeq(r->pptr2->const_at(2).moved, 0);
		UT_ASSERTeq(r->pptr2->const_at(3).moved, 0);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<vector_type>(r->pptr1);
			nvobj::delete_persistent<vector_type>(r->pptr2);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	pop.close();

	return 0;
}
