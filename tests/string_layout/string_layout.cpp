// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

struct root {
};

using char_string = pmem::obj::basic_string<char>;
using char16_string = pmem::obj::basic_string<char16_t>;
using char32_string = pmem::obj::basic_string<char32_t>;
using wchar_string = pmem::obj::basic_string<wchar_t>;

void
test_capacity(pmem::obj::pool<root> &pop)
{
	pmem::obj::transaction::run(pop, [&] {
		auto ptr1 = pmem::obj::make_persistent<char_string>();
		UT_ASSERTeq(ptr1->capacity(), 23);

		pmem::obj::delete_persistent<char_string>(ptr1);
	});

	pmem::obj::transaction::run(pop, [&] {
		auto ptr1 = pmem::obj::make_persistent<char16_string>();
		UT_ASSERTeq(ptr1->capacity(), 11);

		pmem::obj::delete_persistent<char16_string>(ptr1);
	});

	pmem::obj::transaction::run(pop, [&] {
		auto ptr1 = pmem::obj::make_persistent<char32_string>();
		UT_ASSERTeq(ptr1->capacity(), 5);

		pmem::obj::delete_persistent<char32_string>(ptr1);
	});
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = pmem::obj::pool<root>::create(
		path, "StringTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	static_assert(sizeof(char_string) == 32, "");
	static_assert(sizeof(char16_string) == 32, "");
	static_assert(sizeof(char32_string) == 32, "");
	static_assert(sizeof(wchar_string) == 32, "");

	static_assert(std::is_standard_layout<char_string>::value, "");
	static_assert(std::is_standard_layout<char16_string>::value, "");
	static_assert(std::is_standard_layout<char32_string>::value, "");
	static_assert(std::is_standard_layout<wchar_string>::value, "");

	test_capacity(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
