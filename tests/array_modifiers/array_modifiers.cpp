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

#include <algorithm>
#include <iterator>

#include <libpmemobj++/container/array.hpp>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using array_type = pmem::obj::array<double, 5>;
using array_move_type = pmem::obj::array<move_only, 5>;
using array_str = pmem::obj::array<pmem::obj::string, 10>;

struct root {
	pmem::obj::persistent_ptr<array_type> ptr_a;
	pmem::obj::persistent_ptr<array_type> ptr_b;

	pmem::obj::persistent_ptr<array_move_type> ptr_c;
	pmem::obj::persistent_ptr<array_move_type> ptr_d;
	pmem::obj::persistent_ptr<array_str> ptr_str;
};

void
test_modifiers(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_a = pmem::obj::make_persistent<array_type>();
			r->ptr_b = pmem::obj::make_persistent<array_type>();

			r->ptr_c =
				pmem::obj::make_persistent<array_move_type>();
			r->ptr_d =
				pmem::obj::make_persistent<array_move_type>();
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	r->ptr_a->fill(2.4);
	r->ptr_b->fill(1.0);

	r->ptr_a->swap(*(r->ptr_b));

	*(r->ptr_a) = *(r->ptr_b);
	*(r->ptr_b) = std::move(*(r->ptr_a));

	*(r->ptr_a) = *(r->ptr_b);

	*(r->ptr_b) = *(r->ptr_b);
	UT_ASSERT(*(r->ptr_a) == *(r->ptr_b));

	*(r->ptr_b) = std::move(*(r->ptr_b));
	UT_ASSERT(*(r->ptr_a) == *(r->ptr_b));

	r->ptr_b->swap(*(r->ptr_b));
	UT_ASSERT(*(r->ptr_a) == *(r->ptr_b));

	try {
		pmem::obj::transaction::run(pop, [&] {
			*(r->ptr_c) = std::move(*(r->ptr_d));
			for (size_t i = 0; i < r->ptr_c->size(); i++)
				UT_ASSERTeq(r->ptr_d->at(i).value, 0);
			for (size_t i = 0; i < r->ptr_c->size(); i++)
				UT_ASSERTeq(r->ptr_c->at(i).value, 1);

			pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	for (size_t i = 0; i < r->ptr_c->size(); i++)
		UT_ASSERT(r->ptr_d->at(i).value == 1);

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<array_type>(r->ptr_a);
			pmem::obj::delete_persistent<array_type>(r->ptr_b);

			pmem::obj::delete_persistent<array_move_type>(r->ptr_c);
			pmem::obj::delete_persistent<array_move_type>(r->ptr_d);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_d =
				pmem::obj::make_persistent<array_move_type>();
			r->ptr_c = pmem::obj::make_persistent<array_move_type>(
				std::move(*(r->ptr_d)));

			for (size_t i = 0; i < r->ptr_c->size(); i++)
				UT_ASSERTeq(r->ptr_d->at(i).value, 0);
			for (size_t i = 0; i < r->ptr_c->size(); i++)
				UT_ASSERTeq(r->ptr_c->at(i).value, 1);

			pmem::obj::delete_persistent<array_move_type>(r->ptr_c);
			pmem::obj::delete_persistent<array_move_type>(r->ptr_d);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/*
	 * All of following tests should fail - calling any array 'modifier'
	 * on object which is not on pmem should throw exception.
	 */
	array_type stack_array;

	try {
		stack_array.fill(1.0);
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		stack_array.swap(*(r->ptr_a));
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		stack_array = *(r->ptr_a);
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		stack_array = std::move(*(r->ptr_a));
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		stack_array.swap(stack_array);
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		/* Workaround for -Wself-assign-overloaded compile error */
		auto &ref = stack_array;

		stack_array = ref;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		array_type &ref = stack_array;

		stack_array = std::move(ref);
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

void
test_snapshotting(pmem::obj::pool<struct root> &pop, bool do_abort)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_str = pmem::obj::make_persistent<array_str>();
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->ptr_str->front() = "0";
			(*r->ptr_str)[1] = "1";
			r->ptr_str->at(2) = "2";

			r->ptr_str->back() = "9";

			for (auto it = r->ptr_str->begin() + 3;
			     it < r->ptr_str->begin() + 5; it++)
				it->assign(std::to_string(
						   std::distance(
							   r->ptr_str->begin(),
							   it))
						   .c_str());

			auto range = r->ptr_str->range(5, 2);
			for (auto it = range.begin(); it < range.end(); it++)
				it->assign(std::to_string(
						   5 +
						   std::distance(range.begin(),
								 it))
						   .c_str());

			auto range2 = r->ptr_str->range(7, 2, 1);
			for (auto it = range2.begin(); it < range2.end(); it++)
				it->assign(std::to_string(
						   7 +
						   std::distance(range2.begin(),
								 it))
						   .c_str());

			if (do_abort)
				pmem::obj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
		UT_ASSERT(do_abort);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	if (do_abort) {
		for (const auto &e : *(r->ptr_str)) {
			UT_ASSERT(e.size() == 0);
		}
	} else {
		int i = 0;
		for (const auto &e : *(r->ptr_str)) {
			UT_ASSERT(e.size() == 1);

			std::cout << e.c_str() << " " << i << std::endl;

			UT_ASSERT(e == std::to_string(i++));
		}
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

	auto pop = pmem::obj::pool<root>::create(
		path, "ArrayTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_modifiers(pop);
	test_snapshotting(pop, false);
	test_snapshotting(pop, true);

	pop.close();

	return 0;
}
