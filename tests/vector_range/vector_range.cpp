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

#include <libpmemobj++/experimental/slice.hpp>
#include <libpmemobj++/experimental/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmemobj = pmem::obj;
namespace pmemobj_exp = pmemobj::experimental;

using vec_type = pmemobj_exp::vector<int>;

struct root {
	pmemobj::persistent_ptr<vec_type> pptr;
};

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name " << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = pmemobj::pool<root>::create(
		path, "VectorTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	auto r = pop.root();

	try {
		pmemobj::transaction::run(pop, [&] {
			r->pptr = pmemobj::make_persistent<vec_type>(10U, 1);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	vec_type &pmem_vec = *(r->pptr);
	const vec_type &const_pmem_vec = *(r->pptr);

	/* test std::out_of_range exceptions */
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice =
				pmem_vec.range(0, 10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice =
				pmem_vec.crange(0, 10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmemobj::transaction::run(pop, [&] {
			pmemobj_exp::slice<vec_type::const_iterator> slice =
				const_pmem_vec.range(0,
						     10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice =
				pmem_vec.range(0, 10, 3); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	bool exception_thrown = false;
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice = pmem_vec.range(0, 11); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice =
				pmem_vec.range(0, 11, 3); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		pmemobj::transaction::run(pop, [&] {
			pmemobj_exp::slice<vec_type::const_iterator> slice =
				const_pmem_vec.range(0, 11); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice = pmem_vec.crange(0, 11); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	/* test returned values */
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice1 = pmem_vec.range(0, 3);

			UT_ASSERTeq(&pmem_vec.front(), slice1.begin());
			UT_ASSERTeq(&pmem_vec.front() + 3, slice1.end());

			auto slice2 = pmem_vec.range(0, 3, 1);

			UT_ASSERTeq(&pmem_vec.front(), &*slice2.begin());
			UT_ASSERTeq(&pmem_vec.front() + 3, &*slice2.end());

			auto slice3 = pmem_vec.range(0, 10, 11);

			UT_ASSERTeq(&pmem_vec.front(), &*slice3.begin());
			UT_ASSERTeq(&pmem_vec.front() + 10, &*slice3.end());

			pmemobj_exp::slice<vec_type::const_iterator> slice4 =
				const_pmem_vec.range(0, 3);

			UT_ASSERTeq(&const_pmem_vec.front(), slice4.begin());
			UT_ASSERTeq(&const_pmem_vec.front() + 3, slice4.end());

			auto slice5 = pmem_vec.crange(0, 3);

			UT_ASSERTeq(&pmem_vec.front(), slice5.begin());
			UT_ASSERTeq(&pmem_vec.front() + 3, slice5.end());

			pmemobj::delete_persistent<vec_type>(r->pptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
