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
using C = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<C> v1;
	nvobj::persistent_ptr<C> v2;
};

/**
 * Test pmem::obj::experimental::vector modifiers
 *
 * Checks if vector's state is reverted when transaction aborts.
 * Methods under test:
 * - clear()
 * - resize()
 * - resize() with value
 * - swap()
 */
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	UT_ASSERT(r->v1->size() == 100);

	bool exception_thrown = false;

	/* test clear() revert */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->clear();
			UT_ASSERT(r->v1->empty());
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v1->size() == 100);

	nvobj::transaction::run(pop, [&] {
		for (unsigned i = 0; i < 100; ++i) {
			UT_ASSERT((*r->v1)[i] == 1);
		}
	});

	UT_ASSERT(exception_thrown);

	/* test resize() revert */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->resize(50);
			UT_ASSERT(r->v1->size() == 50);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v1->size() == 100);

	nvobj::transaction::run(pop, [&] {
		for (unsigned i = 0; i < 100; ++i) {
			UT_ASSERT((*r->v1)[i] == 1);
		}
	});

	UT_ASSERT(exception_thrown);

	/* test resize() overload with value revert */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->resize(150, 2);
			UT_ASSERT(r->v1->size() == 150);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v1->size() == 100);

	nvobj::transaction::run(pop, [&] {
		for (unsigned i = 0; i < 100; ++i) {
			UT_ASSERT((*r->v1)[i] == 1);
		}
	});

	UT_ASSERT(exception_thrown);

	/* test swap() */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->swap(*r->v2);

			UT_ASSERT(r->v1->size() == 50);
			UT_ASSERT(r->v2->size() == 100);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 50; ++i) {
					UT_ASSERT((*r->v1)[i] == 2);
				}
				for (unsigned i = 0; i < 100; ++i) {
					UT_ASSERT((*r->v2)[i] == 1);
				}
			});

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v1->size() == 100);
	UT_ASSERT(r->v2->size() == 50);

	nvobj::transaction::run(pop, [&] {
		for (unsigned i = 0; i < 100; ++i) {
			UT_ASSERT((*r->v1)[i] == 1);
		}
		for (unsigned i = 0; i < 50; ++i) {
			UT_ASSERT((*r->v2)[i] == 2);
		}
	});

	UT_ASSERT(exception_thrown);
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
		nvobj::pool<root>::create(path, "VectorTest: modifiers_txabort",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<C>(100U, 1);
			r->v2 = nvobj::make_persistent<C>(50U, 2);
		});

		test(pop);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->v1);
			nvobj::delete_persistent<C>(r->v2);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
