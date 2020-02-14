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

struct testcase1 {
	pmem::obj::string s = {'a', 'b', 'c'};
	pmem::obj::string s_long = {
		'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '1', '2',
		'3', '4', '5', '6', '7', '8', '9', '0', '1', '2', '3', '4',
		'5', '6', '7', '8', '9', '0', '1', '2', '3', '4', '5', '6',
		'7', '8', '9', '0', '1', '2', '3', '4', '5', '6', '7', '8',
		'9', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
		'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'};

	void
	run()
	{
		using T = typename pmem::obj::string::traits_type;
		UT_ASSERT(s == "abc");
		UT_ASSERT(
			s_long ==
			"1234567890123456789012345678901234567890123456789012345678901234567890");
		UT_ASSERT(T::compare(s.c_str(), "abc", s.size()) == 0);
		UT_ASSERT(
			T::compare(
				s_long.c_str(),
				"1234567890123456789012345678901234567890123456789012345678901234567890",
				s_long.size()) == 0);
	}
};

struct testcase2 {
	std::wstring s = {L'a', L'b', L'c'};

	void
	run()
	{
		UT_ASSERT(s == L"abc");
	}
};

struct root {
	nvobj::persistent_ptr<testcase1> r1;
	nvobj::persistent_ptr<testcase2> r2;
};

void
run(pmem::obj::pool<root> &pop)
{
	try {
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1 =
				pmem::obj::make_persistent<testcase1>();
			pop.root()->r2 =
				pmem::obj::make_persistent<testcase2>();
		});

		pop.root()->r1->run();
		pop.root()->r2->run();

		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<testcase1>(pop.root()->r1);
			pmem::obj::delete_persistent<testcase2>(pop.root()->r2);
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
		pop = pmem::obj::pool<root>::create(
			path, "copy_assignment.pass", PMEMOBJ_MIN_POOL,
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
