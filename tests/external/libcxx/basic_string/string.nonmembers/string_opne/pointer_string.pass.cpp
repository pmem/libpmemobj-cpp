//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

struct root {
	nvobj::persistent_ptr<pmem::obj::string> s1, s2, s3, s4;
};

template <class S>
void
test(const typename S::value_type *lhs, const S &rhs, bool x)
{
	UT_ASSERT((lhs != rhs) == x);
}

void
run(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<pmem::obj::string>("");
			r->s2 = nvobj::make_persistent<pmem::obj::string>(
				"abcde");
			r->s3 = nvobj::make_persistent<pmem::obj::string>(
				"abcdefghij");
			r->s4 = nvobj::make_persistent<pmem::obj::string>(
				"abcdefghijklmnopqrst");
		});

		test("", *r->s1, false);
		test("", *r->s2, true);
		test("", *r->s3, true);
		test("", *r->s4, true);
		test("abcde", *r->s1, true);
		test("abcde", *r->s2, false);
		test("abcde", *r->s3, true);
		test("abcde", *r->s4, true);
		test("abcdefghij", *r->s1, true);
		test("abcdefghij", *r->s2, true);
		test("abcdefghij", *r->s3, false);
		test("abcdefghij", *r->s4, true);
		test("abcdefghijklmnopqrst", *r->s1, true);
		test("abcdefghijklmnopqrst", *r->s2, true);
		test("abcdefghijklmnopqrst", *r->s3, true);
		test("abcdefghijklmnopqrst", *r->s4, false);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem::obj::string>(r->s1);
			nvobj::delete_persistent<pmem::obj::string>(r->s2);
			nvobj::delete_persistent<pmem::obj::string>(r->s3);
			nvobj::delete_persistent<pmem::obj::string>(r->s4);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::create(path, "basic_string",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
