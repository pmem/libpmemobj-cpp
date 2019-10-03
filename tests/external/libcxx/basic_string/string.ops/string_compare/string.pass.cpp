//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
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
test(const S &s, const S &str, int x)
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
		});

		test(*r->s1, *r->s1, 0);
		test(*r->s1, *r->s2, -5);
		test(*r->s1, *r->s3, -10);
		test(*r->s1, *r->s4, -20);
		test(*r->s2, *r->s1, 5);
		test(*r->s2, *r->s2, 0);
		test(*r->s2, *r->s3, -5);
		test(*r->s2, *r->s4, -15);
		test(*r->s3, *r->s1, 10);
		test(*r->s3, *r->s2, 5);
		test(*r->s3, *r->s3, 0);
		test(*r->s3, *r->s4, -10);
		test(*r->s4, *r->s1, 20);
		test(*r->s4, *r->s2, 15);
		test(*r->s4, *r->s3, 10);
		test(*r->s4, *r->s4, 0);

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

int
main(int argc, char *argv[])
{
	START();

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

	return 0;
}
