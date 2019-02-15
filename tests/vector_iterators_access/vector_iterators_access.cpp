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
using C = pmem_exp::vector<int>;

struct root {
	nvobj::persistent_ptr<C> v;
};

/* Check if access method can be called out of transaction scope */
void
check_access_out_of_tx(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		r->v->const_at(0);
		r->v->cdata();
		r->v->cfront();
		r->v->cback();
		r->v->cbegin();
		r->v->cend();
		r->v->crbegin();
		r->v->crend();

		(*r->v)[0];
		r->v->at(0);
		r->v->data();
		r->v->front();
		r->v->back();
		r->v->begin();
		r->v->end();
		r->v->rbegin();
		r->v->rend();

		static_cast<const C &>(*r->v).at(0);
		static_cast<const C &>(*r->v).data();
		static_cast<const C &>(*r->v).front();
		static_cast<const C &>(*r->v).back();
		static_cast<const C &>(*r->v).begin();
		static_cast<const C &>(*r->v).end();
		static_cast<const C &>(*r->v).rbegin();
		static_cast<const C &>(*r->v).rend();
		static_cast<const C &>(*r->v)[0];
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/*
 * Check if access methods, iterators and dereference operator add
 * elements to transaction. Expect no pmemcheck errors.
 */
void
check_add_to_tx(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] { (*r->v)[0] = 0; });
		nvobj::transaction::run(pop, [&] { r->v->at(0) = 1; });
		nvobj::transaction::run(pop, [&] {
			auto p = r->v->data();
			for (unsigned i = 0; i < r->v->size(); ++i)
				*(p + i) = 2;
		});
		nvobj::transaction::run(pop, [&] { r->v->front() = 3; });
		nvobj::transaction::run(pop, [&] { r->v->back() = 4; });
		nvobj::transaction::run(pop, [&] { *r->v->begin() = 5; });
		nvobj::transaction::run(pop, [&] { *(r->v->end() - 1) = 6; });
		nvobj::transaction::run(pop, [&] { *r->v->rbegin() = 7; });
		nvobj::transaction::run(pop, [&] { *(r->v->rend() - 1) = 8; });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

void
assert_out_of_range(pmem::obj::pool<struct root> &pop,
		    std::function<void(void)> f)
{
	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] { f(); });
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/*
 * Access element beyond the vector's bounds.
 * Check if std::out_of_range exception is thrown.
 */
void
check_out_of_range(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	auto size = r->v->size();

	try {
		assert_out_of_range(pop, [&] { r->v->at(size); });

		assert_out_of_range(
			pop, [&] { const_cast<const C &>(*r->v).at(size); });

		assert_out_of_range(pop, [&] { r->v->const_at(size); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

void
assert_tx_abort(pmem::obj::pool<struct root> &pop, std::function<void(void)> f)
{
	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			f();
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/* Checks if vector's state is reverted when transaction aborts. */
void
check_tx_abort(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		assert_tx_abort(pop, [&] { (*r->v)[0] = 5; });
		UT_ASSERTeq((*r->v)[0], 1);

		assert_tx_abort(pop, [&] { r->v->at(0) = 5; });
		UT_ASSERT(r->v->at(0) == 1);

		assert_tx_abort(pop, [&] { *r->v->begin() = 5; });
		UT_ASSERT(*r->v->begin() == 1);

		assert_tx_abort(pop, [&] { *(r->v->end() - 1) = 5; });
		UT_ASSERT(*(r->v->end() - 1) == 1);

		assert_tx_abort(pop, [&] { *r->v->rbegin() = 5; });
		UT_ASSERT(*r->v->rbegin() == 1);

		assert_tx_abort(pop, [&] { *(r->v->rend() - 1) = 5; });
		UT_ASSERT(*(r->v->rend() - 1) == 1);

		assert_tx_abort(pop, [&] { r->v->front() = 5; });
		UT_ASSERT(r->v->front() == 1);

		assert_tx_abort(pop, [&] { r->v->back() = 5; });
		UT_ASSERT(r->v->back() == 1);
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
		nvobj::pool<root>::create(path, "VectorTest: iterators",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	nvobj::transaction::run(
		pop, [&] { r->v = nvobj::make_persistent<C>(10U, 1); });

	check_access_out_of_tx(pop);
	check_out_of_range(pop);
	check_tx_abort(pop);
	check_add_to_tx(pop);

	nvobj::delete_persistent_atomic<C>(r->v);

	pop.close();

	return 0;
}
