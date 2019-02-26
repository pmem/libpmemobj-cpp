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

#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem_exp = pmem::obj::experimental;
namespace nvobj = pmem::obj;

struct root {
	nvobj::persistent_ptr<pmem_exp::string> s1, s2, s3, s4;
};

template <class S, class T>
void
test(const S &lhs, const T &rhs, bool x)
{
	UT_ASSERT((lhs == rhs) == x);
}

void
run(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<pmem_exp::string>("");
			r->s2 = nvobj::make_persistent<pmem_exp::string>(
				"abcde");
			r->s3 = nvobj::make_persistent<pmem_exp::string>(
				"abcdefghij");
			r->s4 = nvobj::make_persistent<pmem_exp::string>(
				"abcdefghijklmnopqrst");
		});

		std::string s1(""), s2("abcde"), s3("abcdefghij"),
			s4("abcdefghijklmnopqrst");

		/* test pmem::string with pmem::string comparison */
		test(*r->s1, *r->s1, true);
		test(*r->s1, *r->s2, false);
		test(*r->s1, *r->s3, false);
		test(*r->s1, *r->s4, false);
		test(*r->s2, *r->s1, false);
		test(*r->s2, *r->s2, true);
		test(*r->s2, *r->s3, false);
		test(*r->s2, *r->s4, false);
		test(*r->s3, *r->s1, false);
		test(*r->s3, *r->s2, false);
		test(*r->s3, *r->s3, true);
		test(*r->s3, *r->s4, false);
		test(*r->s4, *r->s1, false);
		test(*r->s4, *r->s2, false);
		test(*r->s4, *r->s3, false);
		test(*r->s4, *r->s4, true);

		/* test pmem::string with std::string comparison */
		test(*r->s1, s1, true);
		test(*r->s1, s2, false);
		test(*r->s1, s3, false);
		test(*r->s1, s4, false);
		test(*r->s2, s1, false);
		test(*r->s2, s2, true);
		test(*r->s2, s3, false);
		test(*r->s2, s4, false);
		test(*r->s3, s1, false);
		test(*r->s3, s2, false);
		test(*r->s3, s3, true);
		test(*r->s3, s4, false);
		test(*r->s4, s1, false);
		test(*r->s4, s2, false);
		test(*r->s4, s3, false);
		test(*r->s4, s4, true);

		/* test std::string with pmem::string comparison */
		test(s1, *r->s1, true);
		test(s1, *r->s2, false);
		test(s1, *r->s3, false);
		test(s1, *r->s4, false);
		test(s2, *r->s1, false);
		test(s2, *r->s2, true);
		test(s2, *r->s3, false);
		test(s2, *r->s4, false);
		test(s3, *r->s1, false);
		test(s3, *r->s2, false);
		test(s3, *r->s3, true);
		test(s3, *r->s4, false);
		test(s4, *r->s1, false);
		test(s4, *r->s2, false);
		test(s4, *r->s3, false);
		test(s4, *r->s4, true);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::string>(r->s1);
			nvobj::delete_persistent<pmem_exp::string>(r->s2);
			nvobj::delete_persistent<pmem_exp::string>(r->s3);
			nvobj::delete_persistent<pmem_exp::string>(r->s4);
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
