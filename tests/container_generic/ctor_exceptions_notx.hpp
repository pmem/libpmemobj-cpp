// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_TESTS_CTOR_NOTX_EXCEPTIONS
#define LIBPMEMOBJ_CPP_TESTS_CTOR_NOTX_EXCEPTIONS

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

/**
 * Test default constructor.
 *
 * Call default constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
template <typename T>
void
test_default_ctor(nvobj::pool_base &pop)
{
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<T> pptr = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr = pmemobj_tx_alloc(sizeof(T),
						pmem::detail::type_num<T>());
			if (pptr == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<T>(&*pptr);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test range constructor.
 *
 * Call range constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
template <typename T>
void
test_iter_iter_ctor(nvobj::pool_base &pop)
{
	using V = typename T::value_type;
	V a[] = {V(), V(), V(), V(), V()};

	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<T> pptr = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr = pmemobj_tx_alloc(sizeof(T),
						pmem::detail::type_num<T>());
			if (pptr == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<T, decltype(std::begin(a)),
				     decltype(std::end(a))>(
			&*pptr, std::begin(a), std::end(a));
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test fill constructor with elements with
 * default values.
 *
 * Call fill constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
template <typename T>
void
test_size_ctor(nvobj::pool_base &pop)
{
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<T> pptr = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr = pmemobj_tx_alloc(sizeof(T),
						pmem::detail::type_num<T>());
			if (pptr == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<T, typename T::size_type>(&*pptr, 100);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test fill constructor with elements with
 * custom values.
 *
 * Call fill constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
template <typename T>
void
test_size_value_ctor(nvobj::pool_base &pop)
{
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<T> pptr = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr = pmemobj_tx_alloc(sizeof(T),
						pmem::detail::type_num<T>());
			if (pptr == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<T, typename T::size_type,
				     typename T::value_type>(&*pptr, 100, 5);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test copy constructor.
 *
 * Call copy constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
template <typename T>
void
test_copy_ctor(nvobj::pool_base &pop)
{
	nvobj::persistent_ptr<T> pptr;

	bool exception_thrown = false;

	try {
		nvobj::transaction::run(
			pop, [&] { pptr = nvobj::make_persistent<T>(); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};

	try {
		nvobj::persistent_ptr<T> pptr2 = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr2 = pmemobj_tx_alloc(sizeof(T),
						 pmem::detail::type_num<T>());
			if (pptr2 == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<T>(&*pptr2, *pptr);
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};
	UT_ASSERT(exception_thrown);

	try {
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<T>(pptr); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};
}

/**
 * Test initializer list constructor.
 *
 * Call initializer list constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
template <typename T>
void
test_initializer_list_ctor(nvobj::pool_base &pop)
{
	using V = typename T::value_type;
	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<T> pptr = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr = pmemobj_tx_alloc(sizeof(T),
						pmem::detail::type_num<T>());
			if (pptr == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<T>(
			&*pptr, std::initializer_list<V>{V(), V(), V(), V()});
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};
	UT_ASSERT(exception_thrown);
}

/**
 * Test move constructor.
 *
 * Call move constructor out of transaction scope.
 * Expect pmem:transaction_scope_error exception is thrown.
 */
template <typename T>
void
test_move_ctor(nvobj::pool_base &pop)
{
	nvobj::persistent_ptr<T> pptr1;

	try {
		nvobj::transaction::run(
			pop, [&] { pptr1 = nvobj::make_persistent<T>(); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};

	bool exception_thrown = false;
	try {
		nvobj::persistent_ptr<T> pptr2 = nullptr;
		nvobj::transaction::run(pop, [&] {
			pptr2 = pmemobj_tx_alloc(sizeof(T),
						 pmem::detail::type_num<T>());
			if (pptr2 == nullptr)
				UT_ASSERT(0);
		});
		pmem::detail::create<T>(&*pptr2, std::move(*pptr1));
	} catch (pmem::transaction_scope_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	};
	UT_ASSERT(exception_thrown);
}

#endif
