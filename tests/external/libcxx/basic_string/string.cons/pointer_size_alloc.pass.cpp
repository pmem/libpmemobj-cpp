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
	nvobj::persistent_ptr<pmem_exp::string> s1;
};

template <class charT>
void
test(const charT *s, unsigned n, pmem::obj::pool<root> &pop)
{
	typedef pmem_exp::basic_string<charT, std::char_traits<charT>> S;
	typedef typename S::traits_type T;

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->s1 = nvobj::make_persistent<pmem_exp::string>(s, n);
	});

	auto &s2 = *r->s1;

	UT_ASSERT(s2.size() == n);
	UT_ASSERT(T::compare(s2.c_str(), s, n) == 0);
	UT_ASSERT(s2.capacity() >= s2.size());

	nvobj::transaction::run(pop, [&] {
		UT_ASSERT(s2.c_str() == s2.data());
		UT_ASSERT(s2.c_str() == s2.cdata());
		UT_ASSERT(s2.c_str() ==
			  static_cast<const pmem_exp::string &>(s2).data());
	});

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<pmem_exp::string>(r->s1);
	});
}

void
run(pmem::obj::pool<root> &pop)
{
	try {
		test("", 0, pop);
		test("1", 1, pop);
		test("1234567980", 10, pop);
		test("123456798012345679801234567980123456798012345679801234567980",
		     60, pop);
		test("123456798012345679801234567980123456798012345679801234567980"
		     "123456798012345679801234567980123456798012345679801234567980",
		     120, pop);
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

	pop.close();

	return 0;
}
