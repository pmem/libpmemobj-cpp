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

namespace nvobj = pmem::obj;

using C = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<C> s1;
	nvobj::persistent_ptr<C> s2;
	nvobj::persistent_ptr<C> s3;
};

template <class S>
void
test(nvobj::pool<root> &pop, S &s)
{
	const S &cs = s;
	UT_ASSERT(&cs.back() == &cs[cs.size() - 1]);
	UT_ASSERT(&cs.back() == &cs[cs.size() - 1]);
	UT_ASSERT(&s.back() == &s[cs.size() - 1]);
	nvobj::transaction::run(
		pop, [&] { s.back() = typename S::value_type('z'); });
	UT_ASSERT(s.back() == typename S::value_type('z'));
	UT_ASSERT(s.back() == s.cback());
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "StringTest: back", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<C>("1");
			r->s2 = nvobj::make_persistent<C>(
				"1234567890123456789012345678901234567890");
			r->s3 = nvobj::make_persistent<C>(
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890");
		});

		test(pop, *r->s1);
		test(pop, *r->s2);
		test(pop, *r->s3);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->s1);
			nvobj::delete_persistent<C>(r->s2);
			nvobj::delete_persistent<C>(r->s3);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
