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
#include <libpmemobj++/make_persistent_atomic.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using vector_type = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v_pptr;
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
	auto pop =
		nvobj::pool<root>::create(path, "VectorTest: iterators",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->v_pptr = nvobj::make_persistent<vector_type>(10U, 5);
	});

	/* check if access method can be called out of transaction scope */
	try {
		auto a1 = r->v_pptr->const_at(0);
		(void)a1;
		auto a2 = r->v_pptr->cdata();
		(void)a2;
		auto a3 = r->v_pptr->cfront();
		(void)a3;
		auto a4 = r->v_pptr->cback();
		(void)a4;
		auto a5 = r->v_pptr->cbegin();
		(void)a5;
		auto a6 = r->v_pptr->cend();
		(void)a6;
		auto a7 = r->v_pptr->crbegin();
		(void)a7;
		auto a8 = r->v_pptr->crend();
		(void)a8;

		auto a9 = static_cast<const vector_type &>(*r->v_pptr)
				  .const_at(0);
		(void)a9;
		auto a10 = static_cast<const vector_type &>(*r->v_pptr).data();
		(void)a10;
		auto a11 = static_cast<const vector_type &>(*r->v_pptr).front();
		(void)a11;
		auto a12 = static_cast<const vector_type &>(*r->v_pptr).back();
		(void)a12;
		auto a13 = static_cast<const vector_type &>(*r->v_pptr).begin();
		(void)a13;
		auto a14 = static_cast<const vector_type &>(*r->v_pptr).end();
		(void)a14;
		auto a15 =
			static_cast<const vector_type &>(*r->v_pptr).rbegin();
		(void)a15;
		auto a16 = static_cast<const vector_type &>(*r->v_pptr).rend();
		(void)a16;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/*
	 * Check if access methods, iterators and dereference operator add
	 * elements to transaction. Expect no pmemcheck errors.
	 */
	try {
		nvobj::transaction::run(pop, [&] { (*r->v_pptr)[0] = 0; });
		nvobj::transaction::run(pop, [&] { r->v_pptr->at(0) = 1; });
		nvobj::transaction::run(pop, [&] {
			auto p = r->v_pptr->data();
			for (unsigned i = 0; i < r->v_pptr->size(); ++i)
				*(p + i) = 2;
		});
		nvobj::transaction::run(pop, [&] { r->v_pptr->front() = 3; });
		nvobj::transaction::run(pop, [&] { r->v_pptr->back() = 4; });
		nvobj::transaction::run(pop, [&] { *r->v_pptr->begin() = 5; });
		nvobj::transaction::run(pop, [&] { *r->v_pptr->rend() = 6; });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	nvobj::delete_persistent_atomic<vector_type>(r->v_pptr);

	pop.close();

	return 0;
}
