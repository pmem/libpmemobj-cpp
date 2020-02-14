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
	nvobj::persistent_ptr<pmem::obj::string> s1;
};

template <class charT>
void
test(unsigned n, charT c, pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->s1 = nvobj::make_persistent<pmem::obj::string>(n, c);
	});

	auto &s2 = *r->s1;

	UT_ASSERT(s2.size() == n);
	nvobj::transaction::run(pop, [&] {
		for (unsigned i = 0; i < n; ++i)
			UT_ASSERT(s2[i] == c);
	});
	UT_ASSERT(s2.capacity() >= s2.size());

	nvobj::transaction::run(pop, [&] {
		UT_ASSERT(s2.c_str() == s2.data());
		UT_ASSERT(s2.c_str() == s2.cdata());
		UT_ASSERT(s2.c_str() ==
			  static_cast<const pmem::obj::string &>(s2).data());
	});

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<pmem::obj::string>(r->s1);
	});
}

void
run(pmem::obj::pool<root> &pop)
{
	try {
		test(0, 'a', pop);
		test(1, 'a', pop);
		test(10, 'a', pop);
		test(100, 'a', pop);
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
		pop = pmem::obj::pool<root>::create(path, "string_test",
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
