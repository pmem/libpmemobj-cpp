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

#include <vector>

namespace nvobj = pmem::obj;
namespace pmem_exp = nvobj::experimental;
using C = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<C> v;
};

void
check_vector(nvobj::pool<struct root> &pop, size_t count, int value)
{
	auto r = pop.root();

	UT_ASSERTeq(r->v->size(), count);

	for (unsigned i = 0; i < count; ++i) {
		UT_ASSERTeq((*r->v)[i], value);
	}
}

/**
 * Test pmem::obj::experimental::vector assign() methods
 *
 * Checks if vector's state is reverted when transaction aborts.
 * Methods under test:
 * - fill version of assign()
 * - range version of assign()
 * - initializer list version of assign()
 * - copy assignment operator
 * - move assignment operator
 * - initializer list assignment operator
 */
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	check_vector(pop, 10, 1);

	/* assign() - fill version */
	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			r->v->assign(100, 2);
			check_vector(pop, 100, 2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - range version */
	exception_thrown = false;
	std::vector<int> v2(100, 2);
	try {
		nvobj::transaction::run(pop, [&] {
			r->v->assign(v2.begin(), v2.end());
			check_vector(pop, 100, 2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - initializer list version */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			r->v->assign({2, 2, 2, 2, 2});
			check_vector(pop, 5, 2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - copy version */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			r->v->assign(*v2);
			check_vector(pop, 100, 2);
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* assign() - move version */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			r->v->assign(std::move(*v2));
			check_vector(pop, 100, 2);
			UT_ASSERT(v2->empty());
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* copy assignment operator */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			*r->v = *v2;
			check_vector(pop, 100, 2);
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* move assignment operator */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			*r->v = std::move(*v2);
			check_vector(pop, 100, 2);
			UT_ASSERT(v2->empty());
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);

	/* initializer list assignment operator */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v2 =
				nvobj::make_persistent<C>(100U, 2);
			*r->v = {2, 2, 2, 2, 2};
			check_vector(pop, 5, 2);
			nvobj::delete_persistent<C>(v2);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
	check_vector(pop, 10, 1);
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
		nvobj::pool<root>::create(path, "VectorTest: assign_txabort",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v = nvobj::make_persistent<C>(10U, 1); });

		test(pop);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
