// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using vector_type = container_t<int>;

/**
 * Test pmem::obj::vector default constructor.
 *
 * Call default constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
void
test_default_ctor(nvobj::pool<struct root> &pop)
{
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<vector_type> pptr_v = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr_v = pmemobj_tx_alloc(
				sizeof(vector_type),
				pmem::detail::type_num<vector_type>());
			if (pptr_v == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<vector_type>(&*pptr_v);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::vector range constructor.
 *
 * Call range constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
void
test_iter_iter_ctor(nvobj::pool<struct root> &pop)
{
	int a[] = {0, 1, 2, 3, 4, 5};

	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<vector_type> pptr_v = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr_v = pmemobj_tx_alloc(
				sizeof(vector_type),
				pmem::detail::type_num<vector_type>());
			if (pptr_v == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<vector_type, decltype(std::begin(a)),
				     decltype(std::end(a))>(
			&*pptr_v, std::begin(a), std::end(a));
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::vector fill constructor with elements with
 * default values.
 *
 * Call fill constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
void
test_size_ctor(nvobj::pool<struct root> &pop)
{
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<vector_type> pptr_v = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr_v = pmemobj_tx_alloc(
				sizeof(vector_type),
				pmem::detail::type_num<vector_type>());
			if (pptr_v == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<vector_type, vector_type::size_type>(
			&*pptr_v, 100);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::vector fill constructor with elements with
 * custom values.
 *
 * Call fill constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
void
test_size_value_ctor(nvobj::pool<struct root> &pop)
{
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<vector_type> pptr_v = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr_v = pmemobj_tx_alloc(
				sizeof(vector_type),
				pmem::detail::type_num<vector_type>());
			if (pptr_v == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<vector_type, vector_type::size_type,
				     vector_type::value_type>(&*pptr_v, 100, 5);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::vector copy constructor.
 *
 * Call copy constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
void
test_copy_ctor(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<vector_type> pptr;

	bool exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&] {
			pptr = nvobj::make_persistent<vector_type>();
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};

	try {
		nvobj::persistent_ptr<vector_type> pptr_v = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr_v = pmemobj_tx_alloc(
				sizeof(vector_type),
				pmem::detail::type_num<vector_type>());
			if (pptr_v == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<vector_type>(&*pptr_v, *pptr);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};
	UT_ASSERT(exception_thrown);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<vector_type>(pptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};
}

/**
 * Test pmem::obj::vector initializer list constructor.
 *
 * Call initializer list constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
void
test_initializer_list_ctor(nvobj::pool<struct root> &pop)
{
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<vector_type> pptr_v = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr_v = pmemobj_tx_alloc(
				sizeof(vector_type),
				pmem::detail::type_num<vector_type>());
			if (pptr_v == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<vector_type>(
			&*pptr_v, std::initializer_list<int>{1, 2, 3, 4});
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};
	UT_ASSERT(exception_thrown);
}

/**
 * Test pmem::obj::vector move constructor.
 *
 * Call move constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
void
test_move_ctor(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<vector_type> pptr;

	try {
		nvobj::transaction::run(pop, [&] {
			pptr = nvobj::make_persistent<vector_type>();
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};

	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<vector_type> pptr_v = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr_v = pmemobj_tx_alloc(
				sizeof(vector_type),
				pmem::detail::type_num<vector_type>());
			if (pptr_v == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<vector_type>(&*pptr_v, std::move(*pptr));
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};
	UT_ASSERT(exception_thrown);
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}
	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: vector_ctor_exceptions_notx",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	test_copy_ctor(pop);
	test_default_ctor(pop);
	test_initializer_list_ctor(pop);
	test_iter_iter_ctor(pop);
	test_move_ctor(pop);
	test_size_ctor(pop);
	test_size_value_ctor(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
