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
	nvobj::persistent_ptr<pmem_exp::string> s;
	nvobj::persistent_ptr<pmem_exp::wstring> ws;
};

template <class S>
void
test(S &s1, pmem::obj::pool<root> &pop, nvobj::persistent_ptr<S> &ptr)
{
	using T = typename S::traits_type;

	nvobj::transaction::run(pop,
				[&] { ptr = nvobj::make_persistent<S>(s1); });

	auto &s2 = *ptr;

	UT_ASSERT(s2 == s1);

	UT_ASSERT(s1.size() == s2.size());
	UT_ASSERT(T::compare(s2.c_str(), s1.c_str(), s1.size()) == 0);
	UT_ASSERT(s2.capacity() >= s2.size());

	nvobj::transaction::run(pop, [&] {
		UT_ASSERT(s2.c_str() == s2.data());
		UT_ASSERT(s2.c_str() == s2.cdata());
		UT_ASSERT(s2.c_str() == static_cast<const S &>(s2).data());
	});

	nvobj::transaction::run(pop, [&] { nvobj::delete_persistent<S>(ptr); });
}

void
run(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::persistent_ptr<pmem_exp::string> s1, s2, s3, s4;

		nvobj::transaction::run(pop, [&] {
			s1 = nvobj::make_persistent<pmem_exp::string>();
		});

		nvobj::transaction::run(pop, [&] {
			s2 = nvobj::make_persistent<pmem_exp::string>("1");
		});

		nvobj::transaction::run(pop, [&] {
			s3 = nvobj::make_persistent<pmem_exp::string>(
				"1234567890");
		});

		nvobj::transaction::run(pop, [&] {
			s4 = nvobj::make_persistent<pmem_exp::string>(
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890");
		});

		test(*s1, pop, r->s);
		test(*s2, pop, r->s);
		test(*s3, pop, r->s);
		test(*s4, pop, r->s);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::string>(s1);
			nvobj::delete_persistent<pmem_exp::string>(s2);
			nvobj::delete_persistent<pmem_exp::string>(s3);
			nvobj::delete_persistent<pmem_exp::string>(s4);
		});

	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

void
run_wstring(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::persistent_ptr<pmem_exp::wstring> ws1, ws2, ws3, ws4;

		nvobj::transaction::run(pop, [&] {
			ws1 = nvobj::make_persistent<pmem_exp::wstring>();
		});

		nvobj::transaction::run(pop, [&] {
			ws2 = nvobj::make_persistent<pmem_exp::wstring>(L"1");
		});

		nvobj::transaction::run(pop, [&] {
			ws3 = nvobj::make_persistent<pmem_exp::wstring>(
				L"12345678901234567890");
		});

		nvobj::transaction::run(pop, [&] {
			ws4 = nvobj::make_persistent<pmem_exp::wstring>(
				L"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890");
		});

		test(*ws1, pop, r->ws);
		test(*ws2, pop, r->ws);
		test(*ws3, pop, r->ws);
		test(*ws4, pop, r->ws);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::wstring>(ws1);
			nvobj::delete_persistent<pmem_exp::wstring>(ws2);
			nvobj::delete_persistent<pmem_exp::wstring>(ws3);
			nvobj::delete_persistent<pmem_exp::wstring>(ws4);
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
		pop = pmem::obj::pool<root>::create(path, "string_test",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);
	run_wstring(pop);

	pop.close();

	return 0;
}
