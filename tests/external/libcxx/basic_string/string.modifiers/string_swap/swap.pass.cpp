//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2020, Intel Corporation
//
// Modified to test pmem::obj containers
//
// template <typename CharT, typename Traits>
//   void basic_string<CharT, Traits>::swap(basic_string &other)

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>

#include <string>

using S = pmem::obj::string;

struct root {
	pmem::obj::persistent_ptr<S> lhs, rhs;
};

bool
invariants(const pmem::obj::persistent_ptr<S> &str)
{
	if (str->size() > str->capacity())
		return false;
	if (str->capacity() < S::sso_capacity)
		return false;
	if (str->data() == 0)
		return false;
	if (str->data()[str->size()] != S::value_type(0))
		return false;
	return true;
}

template <class S>
void
test_swap(pmem::obj::persistent_ptr<S> &lhs, pmem::obj::persistent_ptr<S> &rhs,
	  std::string s1, std::string s2)
{
	lhs->assign(s1);
	rhs->assign(s2);
	lhs->swap(*rhs);
	UT_ASSERT(invariants(lhs));
	UT_ASSERT(invariants(rhs));
	UT_ASSERT(*rhs == s1);
	UT_ASSERT(rhs->size() == s1.size());
	UT_ASSERT(*lhs == s2);
	UT_ASSERT(lhs->size() == s2.size());
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = pmem::obj::pool<root>::create(
		path, "string_swap_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto &lhs = pop.root()->lhs;
	auto &rhs = pop.root()->rhs;

	try {
		pmem::obj::transaction::run(pop, [&] {
			lhs = pmem::obj::make_persistent<S>();
			rhs = pmem::obj::make_persistent<S>();
		});

		test_swap(lhs, rhs, "", "");
		test_swap(lhs, rhs, "", "12345");
		test_swap(lhs, rhs, "", "1234567890");
		test_swap(lhs, rhs, "", "12345678901234567890");
		test_swap(lhs, rhs, "abcde", "");
		test_swap(lhs, rhs, "abcde", "12345");
		test_swap(lhs, rhs, "abcde", "1234567890");
		test_swap(lhs, rhs, "abcde", "12345678901234567890");
		test_swap(lhs, rhs, "abcdefghij", "");
		test_swap(lhs, rhs, "abcdefghij", "12345");
		test_swap(lhs, rhs, "abcdefghij", "1234567890");
		test_swap(lhs, rhs, "abcdefghij", "12345678901234567890");
		test_swap(lhs, rhs, "abcdefghijklmnopqrst", "");
		test_swap(lhs, rhs, "abcdefghijklmnopqrst", "12345");
		test_swap(lhs, rhs, "abcdefghijklmnopqrst", "1234567890");
		test_swap(lhs, rhs, "abcdefghijklmnopqrst",
			  "12345678901234567890");
		test_swap(lhs, rhs, std::string(S::sso_capacity * 2, 'a'),
			  std::string(S::sso_capacity * 2, 'b'));
		test_swap(lhs, rhs, std::string(S::sso_capacity * 4, 'a'),
			  std::string(S::sso_capacity * 2, 'b'));
		test_swap(lhs, rhs, std::string(S::sso_capacity * 2, 'a'),
			  std::string(S::sso_capacity * 4, 'b'));
		test_swap(lhs, rhs, std::string(S::sso_capacity * 4, 'a'),
			  std::string(S::sso_capacity * 4, 'b'));

		/* XXX: add swap test cases when allocator template will be
		 * added to pmem::obj::basic_string */

		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<S>(lhs);
			pmem::obj::delete_persistent<S>(rhs);
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
