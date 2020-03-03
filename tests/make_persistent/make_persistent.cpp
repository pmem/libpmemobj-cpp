// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * obj_cpp_make_persistent.cpp -- cpp make_persistent test for objects
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
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

	foo(int val) : bar(val)
	{
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			this->arr[i] = static_cast<char>(val);
	}

	foo(int val, char arr_val) : bar(val)
	{
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			this->arr[i] = arr_val;
	}

	/*
	 * Assert values of foo.
	 */
	void
	check_foo(int val, char arr_val)
	{
		UT_ASSERTeq(val, this->bar);
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			UT_ASSERTeq(arr_val, this->arr[i]);
	}

	nvobj::p<int> bar;
	nvobj::p<char> arr[TEST_ARR_SIZE];
};

struct big_struct {
	char data[PMEMOBJ_MIN_POOL];
};

struct struct_throwing {
	struct_throwing()
	{
		throw magic_number;
	}

	char data[8];
	static constexpr int magic_number = 42;
};

struct root {
	nvobj::persistent_ptr<foo> pfoo;
	nvobj::persistent_ptr<big_struct> bstruct;
	nvobj::persistent_ptr<struct_throwing> throwing;
};

/*
 * test_make_no_args -- (internal) test make_persistent without arguments
 */
void
test_make_no_args(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);

			r->pfoo = nvobj::make_persistent<foo>();
			r->pfoo->check_foo(1, 1);

			nvobj::delete_persistent<foo>(r->pfoo);
			r->pfoo = nullptr;
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->pfoo == nullptr);
}

/*
 * test_make_args -- (internal) test make_persistent with arguments
 */
void
test_make_args(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);

			r->pfoo = nvobj::make_persistent<foo>(2);
			r->pfoo->check_foo(2, 2);

			nvobj::delete_persistent<foo>(r->pfoo);

			r->pfoo = nvobj::make_persistent<foo>(3, 4);
			r->pfoo->check_foo(3, 4);

			nvobj::delete_persistent<foo>(r->pfoo);
			r->pfoo = nullptr;
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->pfoo == nullptr);
}

/*
 * test_additional_delete -- (internal) test double delete and delete rollback
 */
void
test_additional_delete(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);

			r->pfoo = nvobj::make_persistent<foo>();
			r->pfoo->check_foo(1, 1);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo != nullptr);
			nvobj::delete_persistent<foo>(r->pfoo);
			r->pfoo = nullptr;
			nvobj::delete_persistent<foo>(r->pfoo);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->pfoo != nullptr);
	r->pfoo->check_foo(1, 1);

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo != nullptr);
			nvobj::delete_persistent<foo>(r->pfoo);
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
		r->pfoo = nvobj::make_persistent<foo>();
		UT_ASSERT(0);
	} catch (pmem::transaction_scope_error &) {
		scope_error_thrown = true;
	}
	UT_ASSERT(scope_error_thrown);

	/* OOM handling */
	bool alloc_error_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->bstruct == nullptr);

			r->bstruct = nvobj::make_persistent<big_struct>();
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
			UT_ASSERT(r->bstruct == nullptr);

			r->bstruct = nvobj::make_persistent<big_struct>();
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
			UT_ASSERT(r->bstruct == nullptr);

			r->bstruct = nvobj::make_persistent<big_struct>();
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

			r->pfoo = nvobj::make_persistent<foo>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}
	try {
		/* Run outside of a transaction, expect error */
		nvobj::delete_persistent<foo>(r->pfoo);
		UT_ASSERT(0);
	} catch (pmem::transaction_scope_error &) {
		scope_error_delete_thrown = true;
	}
	UT_ASSERT(scope_error_delete_thrown);

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->throwing == nullptr);

			r->throwing = nvobj::make_persistent<struct_throwing>();
			UT_ASSERT(0);
		});
	} catch (int &e) {
		UT_ASSERT(e == struct_throwing::magic_number);
	}
	UT_ASSERT(r->throwing == nullptr);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<foo>(r->pfoo);
			r->pfoo = nullptr;
		});
	} catch (...) {
		UT_ASSERT(0);
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
		{sizeof(foo) + 16, 0, 200, POBJ_HEADER_COMPACT, 0});

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);
			r->pfoo = nvobj::make_persistent<foo>(
				nvobj::allocation_flag::class_id(
					alloc_class.class_id));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(pmemobj_alloc_usable_size(r->pfoo.raw()), sizeof(foo));

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<foo>(r->pfoo);
			r->pfoo = nullptr;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->pfoo == nullptr);
			r->pfoo = nvobj::make_persistent<foo>(
				nvobj::allocation_flag::class_id(
					alloc_class.class_id),
				1, 2);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	r->pfoo->check_foo(1, 2);

	UT_ASSERTeq(pmemobj_alloc_usable_size(r->pfoo.raw()), sizeof(foo));

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<foo>(r->pfoo);
			r->pfoo = nullptr;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<struct root> pop;

	try {
		pop = nvobj::pool<root>::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_make_no_args(pop);
	test_make_args(pop);
	test_additional_delete(pop);
	test_exceptions_handling(pop);
	test_flags(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
