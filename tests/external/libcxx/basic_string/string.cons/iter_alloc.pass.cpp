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

template <class It>
void
test(It first, It last, pmem::obj::pool<root> &pop)
{
	typedef typename std::iterator_traits<It>::value_type charT;
	typedef pmem_exp::basic_string<charT, std::char_traits<charT>> S;

	auto r = pop.root();

	pmem::obj::transaction::run(
		pop, [&] { r->s1 = nvobj::make_persistent<S>(first, last); });

	auto &s2 = *r->s1;

	UT_ASSERT(s2.size() ==
		  static_cast<std::size_t>(std::distance(first, last)));
	unsigned i = 0;
	pmem::obj::transaction::run(pop, [&] {
		for (It it = first; it != last; ++it, ++i)
			UT_ASSERT(s2[i] == *it);
		UT_ASSERT(s2.capacity() >= s2.size());
	});

	nvobj::transaction::run(pop, [&] {
		UT_ASSERT(s2.c_str() == s2.data());
		UT_ASSERT(s2.c_str() == s2.cdata());
		UT_ASSERT(s2.c_str() ==
			  static_cast<const pmem_exp::string &>(s2).data());
	});

	pmem::obj::transaction::run(
		pop, [&] { nvobj::delete_persistent<S>(r->s1); });
}

void
run(pmem::obj::pool<root> &pop)
{
	const char *s =
		"123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";

	try {
		test(s, s, pop);
		test(s, s + 1, pop);
		test(s, s + 10, pop);
		test(s, s + 50, pop);
		test(s, s + 70, pop);
		test(test_support::input_it<const char *>(s),
		     test_support::input_it<const char *>(s), pop);
		test(test_support::input_it<const char *>(s),
		     test_support::input_it<const char *>(s + 1), pop);
		test(test_support::input_it<const char *>(s),
		     test_support::input_it<const char *>(s + 10), pop);
		test(test_support::input_it<const char *>(s),
		     test_support::input_it<const char *>(s + 50), pop);
		test(test_support::input_it<const char *>(s),
		     test_support::input_it<const char *>(s + 70), pop);
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
		pop = pmem::obj::pool<root>::create(path, "iter_alloc.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	pop.close();

	return 0;
}
