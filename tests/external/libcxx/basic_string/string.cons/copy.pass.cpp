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

using T = pmem_exp::string;
using W = pmem_exp::wstring;

struct root {
	nvobj::persistent_ptr<T> s;
	nvobj::persistent_ptr<W> ws;
};

template <class S, class U>
void
test(U &s1, pmem::obj::pool<root> &pop, nvobj::persistent_ptr<S> &ptr)
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

	/* test pmem::string copy construction from pmem::string */
	try {
		nvobj::persistent_ptr<T> s1, s2, s3, s4;

		nvobj::transaction::run(pop, [&] {
			s1 = nvobj::make_persistent<T>();
			s2 = nvobj::make_persistent<T>("1");
			s3 = nvobj::make_persistent<T>("1234567890");
			s4 = nvobj::make_persistent<T>(
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890");
		});

		test<T>(*s1, pop, r->s);
		test<T>(*s2, pop, r->s);
		test<T>(*s3, pop, r->s);
		test<T>(*s4, pop, r->s);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<T>(s1);
			nvobj::delete_persistent<T>(s2);
			nvobj::delete_persistent<T>(s3);
			nvobj::delete_persistent<T>(s4);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/* test pmem::string copy construction from std::string */
	try {
		std::string s1, s2("1"), s3("1234567890"),
			s4("1234567890123456789012345678901234567890"
			   "1234567890123456789012345678901234567890"
			   "1234567890123456789012345678901234567890"
			   "1234567890");

		test<T>(s1, pop, r->s);
		test<T>(s2, pop, r->s);
		test<T>(s3, pop, r->s);
		test<T>(s4, pop, r->s);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

void
run_wstring(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	/* test pmem::wstring copy construction from pmem::wstring */
	try {
		nvobj::persistent_ptr<W> ws1, ws2, ws3, ws4;

		nvobj::transaction::run(pop, [&] {
			ws1 = nvobj::make_persistent<W>();
			ws2 = nvobj::make_persistent<W>(L"1");
			ws3 = nvobj::make_persistent<W>(
				L"12345678901234567890");
			ws4 = nvobj::make_persistent<W>(
				L"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890");
		});

		test<W>(*ws1, pop, r->ws);
		test<W>(*ws2, pop, r->ws);
		test<W>(*ws3, pop, r->ws);
		test<W>(*ws4, pop, r->ws);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<W>(ws1);
			nvobj::delete_persistent<W>(ws2);
			nvobj::delete_persistent<W>(ws3);
			nvobj::delete_persistent<W>(ws4);
		});

	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	/* test pmem::wstring copy construction from std::wstring */
	try {
		std::wstring ws1, ws2(L"1"), ws3(L"12345678901234567890"),
			ws4(L"1234567890123456789012345678901234567890"
			    "1234567890123456789012345678901234567890"
			    "1234567890123456789012345678901234567890"
			    "1234567890");

		test<W>(ws1, pop, r->ws);
		test<W>(ws2, pop, r->ws);
		test<W>(ws3, pop, r->ws);
		test<W>(ws4, pop, r->ws);
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
