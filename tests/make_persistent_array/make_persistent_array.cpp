/*
 * Copyright 2016-2019, Intel Corporation
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

/*
 * obj_cpp_make_persistent_array.cpp -- cpp make_persistent test for arrays
 */

#include "unittest.hpp"

#include <iostream>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj/ctl.h>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

const int TEST_ARR_SIZE = 10;

class foo {
public:
	foo() : bar(1)
	{
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			this->arr[i] = 1;
	}

	/*
	 * Assert values of foo.
	 */
	void
	check_foo()
	{
		UT_ASSERTeq(1, this->bar);
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			UT_ASSERTeq(1, this->arr[i]);
	}

	~foo()
	{
		this->bar = 0;
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			this->arr[i] = 0;
	}

	nvobj::p<int> bar;
	nvobj::p<char> arr[TEST_ARR_SIZE];
};

int ctor_number = 0;

struct struct_throwing {
	struct_throwing()
	{
		if (ctor_number == throw_after)
			throw magic_number;

		ctor_number++;
	}

	char data[8];
	static constexpr int magic_number = 42;
	static constexpr int throw_after = 5;
};

struct root {
	nvobj::persistent_ptr<foo[]> pfoo;
	nvobj::persistent_ptr<struct_throwing[]> throwing;

	nvobj::persistent_ptr<foo[10]> pfoo_sized;
	nvobj::persistent_ptr<foo[PMEMOBJ_MIN_POOL]> pfoo_sized_big;
	nvobj::persistent_ptr<struct_throwing[10]> throwing_sized;
};

/*
 * test_make_one_d -- (internal) test make_persistent of a 1d array
 */
void
test_make_one_d(nvobj::pool_base &pop)
{
	try {
		nvobj::transaction::run(pop, [&] {
			auto pfoo = nvobj::make_persistent<foo[]>(5);
			for (nvobj::p<int> i = 0; i < 5; ++i)
				pfoo[i].check_foo();

			nvobj::delete_persistent<foo[]>(pfoo, 5);

			auto pfoo2 = nvobj::make_persistent<foo[]>(6);
			for (int i = 0; i < 6; ++i)
				pfoo2[i].check_foo();

			nvobj::delete_persistent<foo[]>(pfoo2, 6);

			auto pfooN = nvobj::make_persistent<foo[5]>();
			for (int i = 0; i < 5; ++i)
				pfooN[i].check_foo();

			nvobj::delete_persistent<foo[5]>(pfooN);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

/*
 * test_make_N_d -- (internal) test make_persistent of 2d and 3d arrays
 */
void
test_make_N_d(nvobj::pool_base &pop)
{
	try {
		nvobj::transaction::run(pop, [&] {
			auto pfoo = nvobj::make_persistent<foo[][2]>(5);
			for (int i = 0; i < 5; ++i)
				for (int j = 0; j < 2; j++)
					pfoo[i][j].check_foo();

			nvobj::delete_persistent<foo[][2]>(pfoo, 5);

			auto pfoo2 = nvobj::make_persistent<foo[][3]>(6);
			for (int i = 0; i < 6; ++i)
				for (int j = 0; j < 3; j++)
					pfoo2[i][j].check_foo();

			nvobj::delete_persistent<foo[][3]>(pfoo2, 6);

			auto pfooN = nvobj::make_persistent<foo[5][2]>();
			for (int i = 0; i < 5; ++i)
				for (int j = 0; j < 2; j++)
					pfooN[i][j].check_foo();

			nvobj::delete_persistent<foo[5][2]>(pfooN);

			auto pfoo3 = nvobj::make_persistent<foo[][2][3]>(5);
			for (int i = 0; i < 5; ++i)
				for (int j = 0; j < 2; j++)
					for (int k = 0; k < 3; k++)
						pfoo3[i][j][k].check_foo();

			nvobj::delete_persistent<foo[][2][3]>(pfoo3, 5);

			auto pfoo3N = nvobj::make_persistent<foo[5][2][3]>();
			for (int i = 0; i < 5; ++i)
				for (int j = 0; j < 2; j++)
					for (int k = 0; k < 3; k++)
						pfoo3N[i][j][k].check_foo();

			nvobj::delete_persistent<foo[5][2][3]>(pfoo3N);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

/*
 * test_abort_revert -- (internal) test destruction behavior and revert
 */
void
test_abort_revert(nvobj::pool_base &pop)
{
	nvobj::pool<struct root> &root_pop =
		dynamic_cast<nvobj::pool<struct root> &>(pop);
	nvobj::persistent_ptr<root> r = root_pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->pfoo = nvobj::make_persistent<foo[]>(5);
			for (int i = 0; i < 5; ++i)
				r->pfoo[i].check_foo();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo != nullptr);
			nvobj::delete_persistent<foo[]>(r->pfoo, 5);
			r->pfoo = nullptr;

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->pfoo != nullptr);
	for (int i = 0; i < 5; ++i)
		r->pfoo[i].check_foo();

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<foo[]>(r->pfoo, 5);
			r->pfoo = nullptr;
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->pfoo == nullptr);
}

/*
 * test_exceptions_handling -- (internal) test proper handling of exceptions
 * inside make_persistent
 */
void
test_exceptions_handling(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

	bool scope_error_thrown = false;
	try {
		/* Run outside of a transaction, expect error */
		r->pfoo = nvobj::make_persistent<foo[]>(5);
		UT_ASSERT(0);
	} catch (pmem::transaction_scope_error &) {
		scope_error_thrown = true;
	}
	UT_ASSERT(scope_error_thrown);

	/* OOM handling */
	bool alloc_error_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);

			r->pfoo =
				nvobj::make_persistent<foo[]>(PMEMOBJ_MIN_POOL);
			UT_ASSERT(0);
		});
	} catch (pmem::transaction_alloc_error &) {
		alloc_error_thrown = true;
	}
	UT_ASSERT(alloc_error_thrown);

	/* OOM handling */
	alloc_error_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);

			r->pfoo =
				nvobj::make_persistent<foo[]>(PMEMOBJ_MIN_POOL);
			UT_ASSERT(0);
		});
	} catch (pmem::transaction_error &e) {
		(void)e.what();
		alloc_error_thrown = true;
	}
	UT_ASSERT(alloc_error_thrown);

	/* OOM handling */
	alloc_error_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);

			r->pfoo =
				nvobj::make_persistent<foo[]>(PMEMOBJ_MIN_POOL);
			UT_ASSERT(0);
		});
	} catch (std::bad_alloc &e) {
		(void)e.what();
		alloc_error_thrown = true;
	}
	UT_ASSERT(alloc_error_thrown);

	bool scope_error_delete_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);

			r->pfoo = nvobj::make_persistent<foo[]>(5);
		});
	} catch (...) {
		UT_ASSERT(0);
	}
	try {
		/* Run outside of a transaction, expect error */
		nvobj::delete_persistent<foo[]>(r->pfoo, 5);
		UT_ASSERT(0);
	} catch (pmem::transaction_scope_error &) {
		scope_error_delete_thrown = true;
	}
	UT_ASSERT(scope_error_delete_thrown);

	ctor_number = 0;

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->throwing == nullptr);

			r->throwing =
				nvobj::make_persistent<struct_throwing[]>(10);
			UT_ASSERT(0);
		});
	} catch (int &e) {
		UT_ASSERT(e == struct_throwing::magic_number);
	}
}

/*
 * test_exceptions_handling -- (internal) test proper handling of exceptions
 * inside make_persistent, version for sized array
 */
void
test_exceptions_handling_sized(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

	bool scope_error_thrown = false;
	try {
		/* Run outside of a transaction, expect error */
		r->pfoo_sized = nvobj::make_persistent<foo[10]>();
		UT_ASSERT(0);
	} catch (pmem::transaction_scope_error &) {
		scope_error_thrown = true;
	}
	UT_ASSERT(scope_error_thrown);

	/* OOM handling */
	bool alloc_error_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo_sized_big == nullptr);

			r->pfoo_sized_big =
				nvobj::make_persistent<foo[PMEMOBJ_MIN_POOL]>();
			UT_ASSERT(0);
		});
	} catch (pmem::transaction_alloc_error &e) {
		(void)e.what();
		alloc_error_thrown = true;
	}
	UT_ASSERT(alloc_error_thrown);

	/* OOM handling */
	alloc_error_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo_sized_big == nullptr);

			r->pfoo_sized_big =
				nvobj::make_persistent<foo[PMEMOBJ_MIN_POOL]>();
			UT_ASSERT(0);
		});
	} catch (pmem::transaction_error &e) {
		(void)e.what();
		alloc_error_thrown = true;
	}
	UT_ASSERT(alloc_error_thrown);

	/* OOM handling */
	alloc_error_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo_sized_big == nullptr);

			r->pfoo_sized_big =
				nvobj::make_persistent<foo[PMEMOBJ_MIN_POOL]>();
			UT_ASSERT(0);
		});
	} catch (std::bad_alloc &e) {
		(void)e.what();
		alloc_error_thrown = true;
	}
	UT_ASSERT(alloc_error_thrown);

	bool scope_error_delete_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo_sized == nullptr);

			r->pfoo_sized = nvobj::make_persistent<foo[10]>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}
	try {
		/* Run outside of a transaction, expect error */
		nvobj::delete_persistent<foo[10]>(r->pfoo_sized);
		UT_ASSERT(0);
	} catch (pmem::transaction_scope_error &) {
		scope_error_delete_thrown = true;
	}
	UT_ASSERT(scope_error_delete_thrown);

	ctor_number = 0;

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->throwing_sized == nullptr);

			r->throwing_sized =
				nvobj::make_persistent<struct_throwing[10]>();
			UT_ASSERT(0);
		});
	} catch (int &e) {
		UT_ASSERT(e == struct_throwing::magic_number);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<foo[10]>(r->pfoo_sized);
			r->pfoo_sized = nullptr;

			nvobj::delete_persistent<foo[]>(r->pfoo, 10);
			r->pfoo = nullptr;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/*
 * test_flags -- (internal) test proper handling of flags
 */
void
test_flags(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

	auto alloc_class = pop.ctl_set<struct pobj_alloc_class_desc>(
		"heap.alloc_class.new.desc",
		{sizeof(foo), 0, 100, POBJ_HEADER_COMPACT, 0});

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo_sized == nullptr);
			r->pfoo_sized = nvobj::make_persistent<foo[10]>(
				nvobj::allocation_flag::class_id(
					alloc_class.class_id));

			UT_ASSERT(r->pfoo == nullptr);
			r->pfoo = nvobj::make_persistent<foo[]>(
				10,
				nvobj::allocation_flag::class_id(
					alloc_class.class_id));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(pmemobj_alloc_usable_size(r->pfoo.raw()), sizeof(foo) * 10);
	UT_ASSERTeq(pmemobj_alloc_usable_size(r->pfoo_sized.raw()),
		    sizeof(foo) * 10);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<foo[10]>(r->pfoo_sized);
			r->pfoo_sized = nullptr;

			nvobj::delete_persistent<foo[]>(r->pfoo, 10);
			r->pfoo = nullptr;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/*
 * test_nullptr -- (internal) test proper handling of null pointers
 */
void
test_nullptr(nvobj::pool<struct root> &pop)
{
	nvobj::transaction::run(pop, [&] {
		nvobj::persistent_ptr<foo[]> f;
		f = nullptr;
		nvobj::delete_persistent<foo[]>(f, 1);

		nvobj::persistent_ptr<foo[10]> f2;
		f2 = nullptr;
		nvobj::delete_persistent<foo[10]>(f2);
	});
}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<struct root> pop;

	try {
		pop = nvobj::pool<struct root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_make_one_d(pop);
	test_make_N_d(pop);
	test_abort_revert(pop);
	test_exceptions_handling(pop);
	test_exceptions_handling_sized(pop);
	test_flags(pop);
	test_nullptr(pop);

	pop.close();

	return 0;
}
