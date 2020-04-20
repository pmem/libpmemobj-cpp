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
// template <typename CharT, typename Traits>
//   void swap(basic_string<CharT, Traits>& lhs, basic_string<CharT, Traits>&
//   rhs);

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
	/* XXX: add check when __min_cap is defined
	 * if (str->capacity() < __min_cap - 1) return false; */
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
	pmem::obj::swap(*lhs, *rhs);
	assert(invariants(lhs));
	assert(invariants(rhs));
	assert(*rhs == s1);
	assert(rhs->size() == s1.size());
	assert(*lhs == s2);
	assert(lhs->size() == s2.size());
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

		/* XXX: test swap when allocator template will be added to
		 * pmem::obj::basic_string */

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
