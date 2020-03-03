// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>

#include <iostream>

using string_type = pmem::obj::string;

struct root {
	pmem::obj::persistent_ptr<string_type> short_str;
	pmem::obj::persistent_ptr<string_type> long_str;
};

static constexpr char short_c_str_ctor[] = "0987654321";
static constexpr char long_c_str_ctor[] =
	"0987654321098765432109876543210987654321"
	"0987654321098765432109876543210987654321"
	"0987654321098765432109876543210987654321"
	"0987654321";

static constexpr char short_c_str[] = "1234567890";
static constexpr char long_c_str[] = "1234567890123456789012345678901234567890"
				     "1234567890123456789012345678901234567890"
				     "1234567890123456789012345678901234567890"
				     "1234567890";

void
test_string_snapshot(pmem::obj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		pmem::obj::transaction::run(pop, [&] {
			r->short_str = pmem::obj::make_persistent<string_type>(
				short_c_str_ctor);
			r->long_str = pmem::obj::make_persistent<string_type>(
				long_c_str_ctor);

			UT_ASSERTeq(r->short_str->size(),
				    strlen(short_c_str_ctor));
			UT_ASSERTeq(r->long_str->size(),
				    strlen(long_c_str_ctor));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	using T = typename string_type::traits_type;

	try {
		pmem::obj::transaction::run(pop, [&] {
			auto data = r->short_str->data();
			strcpy(data, short_c_str);

			UT_ASSERTeq(T::compare(r->short_str->cdata(),
					       short_c_str,
					       r->short_str->size()),
				    0);
			UT_ASSERTeq(T::length(r->short_str->cdata()),
				    T::length(short_c_str));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			auto data = r->long_str->data();
			strcpy(data, long_c_str);

			UT_ASSERTeq(T::compare(r->long_str->cdata(), long_c_str,
					       r->long_str->size()),
				    0);
			UT_ASSERTeq(T::length(r->long_str->cdata()),
				    T::length(long_c_str));
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<string_type>(r->short_str);
			pmem::obj::delete_persistent<string_type>(r->long_str);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
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

	test_string_snapshot(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
