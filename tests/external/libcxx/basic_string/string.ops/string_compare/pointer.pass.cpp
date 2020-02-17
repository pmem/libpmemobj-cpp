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
	nvobj::persistent_ptr<pmem::obj::string> s1, s2, s3, s4, s5;
};

int
sign(int x)
{
	if (x == 0)
		return 0;
	if (x < 0)
		return -1;
	return 1;
}

template <class S>
void
test(const S &s, const typename S::value_type *str, int x)
{
	UT_ASSERT(sign(s.compare(str)) == sign(x));
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
			r->s5 = nvobj::make_persistent<pmem::obj::string>(
				"12345678901234567890"
				"12345678901234567890"
				"12345678901234567890"
				"1234567890");
		});

		test(*r->s1, "", 0);
		test(*r->s1, "abcde", -5);
		test(*r->s1, "abcdefghij", -10);
		test(*r->s1, "abcdefghijklmnopqrst", -20);
		test(*r->s2, "", 5);
		test(*r->s2, "abcde", 0);
		test(*r->s2, "abcdefghij", -5);
		test(*r->s2, "abcdefghijklmnopqrst", -15);
		test(*r->s3, "", 10);
		test(*r->s3, "abcde", 5);
		test(*r->s3, "abcdefghij", 0);
		test(*r->s3, "abcdefghijklmnopqrst", -10);
		test(*r->s4, "", 20);
		test(*r->s4, "abcde", 15);
		test(*r->s4, "abcdefghij", 10);
		test(*r->s4, "abcdefghijklmnopqrst", 0);
		test(*r->s5, "", 20);
		test(*r->s5, "12345", 15);
		test(*r->s5, "1234567890", 10);
		test(*r->s5,
		     "12345678901234567890"
		     "12345678901234567890"
		     "12345678901234567890"
		     "1234567890",
		     0);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem::obj::string>(r->s1);
			nvobj::delete_persistent<pmem::obj::string>(r->s2);
			nvobj::delete_persistent<pmem::obj::string>(r->s3);
			nvobj::delete_persistent<pmem::obj::string>(r->s4);
			nvobj::delete_persistent<pmem::obj::string>(r->s5);
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
