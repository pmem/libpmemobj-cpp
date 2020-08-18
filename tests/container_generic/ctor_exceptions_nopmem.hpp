// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_TESTS_CTOR_NOPMEM_EXCEPTIONS
#define LIBPMEMOBJ_CPP_TESTS_CTOR_NOPMEM_EXCEPTIONS

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

/**
 * Test default constructor of T.
 *
 * Call default constructor for volatile instance of
 * T Expect pmem::pool_error exception is thrown.
 */
template <typename T>
void
test_default_ctor()
{
	bool exception_thrown = false;
	try {
		T v = {};
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test T range constructor from T::value_type.
 *
 * Call range constructor for volatile instance of
 * T. Expect pmem::pool_error exception is thrown.
 */
template <typename T>
void
test_iter_iter_ctor()
{
	using V = typename T::value_type;
	V a[] = {V(), V(), V(), V(), V()};

	bool exception_thrown = false;
	try {
		T v(std::begin(a), std::end(a));
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test T fill constructor with elements with
 * default values.
 *
 * Call fill constructor for volatile instance of
 * T. Expect pmem::pool_error exception is thrown.
 */
template <typename T>
void
test_size_ctor()
{
	bool exception_thrown = false;
	try {
		T v(100);
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test T fill constructor with elements with
 * custom values.
 *
 * Call fill constructor for volatile instance of
 * T. Expect pmem::pool_error exception is thrown.
 */
template <typename T>
void
test_size_value_ctor()
{
	bool exception_thrown = false;
	try {
		T v(100, 5);
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test T copy constructor
 *
 * Call copy constructor for volatile instance of
 * T. Expect pmem::pool_error exception is thrown.
 */
template <typename T>
void
test_copy_ctor(nvobj::pool_base pop, nvobj::persistent_ptr<T> &ptr)
{
	try {
		nvobj::transaction::run(
			pop, [&] { ptr = nvobj::make_persistent<T>(); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	bool exception_thrown = false;
	try {
		T v(*ptr);
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	try {
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<T>(ptr); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

/**
 * Test T initializer list constructor from T::value_type.
 *
 * Call initializer list constructor for volatile instance of
 * T. Expect pmem::pool_error exception is thrown.
 */
template <typename T>
void
test_initializer_list_ctor()
{
	using V = typename T::value_type;
	bool exception_thrown = false;
	try {
		T v(std::initializer_list<V>{V(), V(), V(), V()});
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

/**
 * Test T move constructor
 *
 * Call move constructor for volatile instance of
 * T. Expect pmem::pool_error exception is thrown.
 */
template <typename T>
void
test_move_ctor(nvobj::pool_base pop, nvobj::persistent_ptr<T> &ptr)
{
	try {
		nvobj::transaction::run(
			pop, [&] { ptr = nvobj::make_persistent<T>(); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	bool exception_thrown = false;
	try {
		T v(std::move(*ptr));
		(void)v;
		UT_ASSERT(0);
	} catch (pmem::pool_error &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	try {
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<T>(ptr); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

#endif
