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

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

struct root {
	nvobj::persistent_ptr<pmem::obj::string> s1, s2, s3, s4;
};

int
sign(int x)
{
	if (x == 0)
		return 0;
	if (x < 0)
		return -1;
	return 1;
}

template <class S>
void
test(const S &s, typename S::size_type pos1, typename S::size_type n1,
     const typename S::value_type *str, int x)
{
	if (pos1 <= s.size())
		UT_ASSERT(sign(s.compare(pos1, n1, str)) == sign(x));
	else {
		try {
			s.compare(pos1, n1, str);
			UT_ASSERT(0);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos1 > s.size());
		}
	}
}

template <class S>
void
test0(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	test(*r->s1, 0, 0, "", 0);
	test(*r->s1, 0, 0, "abcde", -5);
	test(*r->s1, 0, 0, "abcdefghij", -10);
	test(*r->s1, 0, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s1, 0, 1, "", 0);
	test(*r->s1, 0, 1, "abcde", -5);
	test(*r->s1, 0, 1, "abcdefghij", -10);
	test(*r->s1, 0, 1, "abcdefghijklmnopqrst", -20);
	test(*r->s1, 1, 0, "", 0);
	test(*r->s1, 1, 0, "abcde", 0);
	test(*r->s1, 1, 0, "abcdefghij", 0);
	test(*r->s1, 1, 0, "abcdefghijklmnopqrst", 0);
	test(*r->s2, 0, 0, "", 0);
	test(*r->s2, 0, 0, "abcde", -5);
	test(*r->s2, 0, 0, "abcdefghij", -10);
	test(*r->s2, 0, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s2, 0, 1, "", 1);
	test(*r->s2, 0, 1, "abcde", -4);
	test(*r->s2, 0, 1, "abcdefghij", -9);
	test(*r->s2, 0, 1, "abcdefghijklmnopqrst", -19);
	test(*r->s2, 0, 2, "", 2);
	test(*r->s2, 0, 2, "abcde", -3);
	test(*r->s2, 0, 2, "abcdefghij", -8);
	test(*r->s2, 0, 2, "abcdefghijklmnopqrst", -18);
	test(*r->s2, 0, 4, "", 4);
	test(*r->s2, 0, 4, "abcde", -1);
	test(*r->s2, 0, 4, "abcdefghij", -6);
	test(*r->s2, 0, 4, "abcdefghijklmnopqrst", -16);
	test(*r->s2, 0, 5, "", 5);
	test(*r->s2, 0, 5, "abcde", 0);
	test(*r->s2, 0, 5, "abcdefghij", -5);
	test(*r->s2, 0, 5, "abcdefghijklmnopqrst", -15);
	test(*r->s2, 0, 6, "", 5);
	test(*r->s2, 0, 6, "abcde", 0);
	test(*r->s2, 0, 6, "abcdefghij", -5);
	test(*r->s2, 0, 6, "abcdefghijklmnopqrst", -15);
	test(*r->s2, 1, 0, "", 0);
	test(*r->s2, 1, 0, "abcde", -5);
	test(*r->s2, 1, 0, "abcdefghij", -10);
	test(*r->s2, 1, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s2, 1, 1, "", 1);
	test(*r->s2, 1, 1, "abcde", 1);
	test(*r->s2, 1, 1, "abcdefghij", 1);
	test(*r->s2, 1, 1, "abcdefghijklmnopqrst", 1);
	test(*r->s2, 1, 2, "", 2);
	test(*r->s2, 1, 2, "abcde", 1);
	test(*r->s2, 1, 2, "abcdefghij", 1);
	test(*r->s2, 1, 2, "abcdefghijklmnopqrst", 1);
	test(*r->s2, 1, 3, "", 3);
	test(*r->s2, 1, 3, "abcde", 1);
	test(*r->s2, 1, 3, "abcdefghij", 1);
	test(*r->s2, 1, 3, "abcdefghijklmnopqrst", 1);
	test(*r->s2, 1, 4, "", 4);
	test(*r->s2, 1, 4, "abcde", 1);
	test(*r->s2, 1, 4, "abcdefghij", 1);
	test(*r->s2, 1, 4, "abcdefghijklmnopqrst", 1);
	test(*r->s2, 1, 5, "", 4);
	test(*r->s2, 1, 5, "abcde", 1);
	test(*r->s2, 1, 5, "abcdefghij", 1);
	test(*r->s2, 1, 5, "abcdefghijklmnopqrst", 1);
	test(*r->s2, 2, 0, "", 0);
	test(*r->s2, 2, 0, "abcde", -5);
	test(*r->s2, 2, 0, "abcdefghij", -10);
	test(*r->s2, 2, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s2, 2, 1, "", 1);
	test(*r->s2, 2, 1, "abcde", 2);
	test(*r->s2, 2, 1, "abcdefghij", 2);
	test(*r->s2, 2, 1, "abcdefghijklmnopqrst", 2);
	test(*r->s2, 2, 2, "", 2);
	test(*r->s2, 2, 2, "abcde", 2);
	test(*r->s2, 2, 2, "abcdefghij", 2);
	test(*r->s2, 2, 2, "abcdefghijklmnopqrst", 2);
	test(*r->s2, 2, 3, "", 3);
	test(*r->s2, 2, 3, "abcde", 2);
	test(*r->s2, 2, 3, "abcdefghij", 2);
	test(*r->s2, 2, 3, "abcdefghijklmnopqrst", 2);
	test(*r->s2, 2, 4, "", 3);
	test(*r->s2, 2, 4, "abcde", 2);
	test(*r->s2, 2, 4, "abcdefghij", 2);
	test(*r->s2, 2, 4, "abcdefghijklmnopqrst", 2);
	test(*r->s2, 4, 0, "", 0);
	test(*r->s2, 4, 0, "abcde", -5);
	test(*r->s2, 4, 0, "abcdefghij", -10);
	test(*r->s2, 4, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s2, 4, 1, "", 1);
	test(*r->s2, 4, 1, "abcde", 4);
	test(*r->s2, 4, 1, "abcdefghij", 4);
	test(*r->s2, 4, 1, "abcdefghijklmnopqrst", 4);
	test(*r->s2, 4, 2, "", 1);
	test(*r->s2, 4, 2, "abcde", 4);
	test(*r->s2, 4, 2, "abcdefghij", 4);
	test(*r->s2, 4, 2, "abcdefghijklmnopqrst", 4);
	test(*r->s2, 5, 0, "", 0);
	test(*r->s2, 5, 0, "abcde", -5);
	test(*r->s2, 5, 0, "abcdefghij", -10);
	test(*r->s2, 5, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s2, 5, 1, "", 0);
	test(*r->s2, 5, 1, "abcde", -5);
	test(*r->s2, 5, 1, "abcdefghij", -10);
	test(*r->s2, 5, 1, "abcdefghijklmnopqrst", -20);
}

template <class S>
void
test1(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	test(*r->s2, 6, 0, "", 0);
	test(*r->s2, 6, 0, "abcde", 0);
	test(*r->s2, 6, 0, "abcdefghij", 0);
	test(*r->s2, 6, 0, "abcdefghijklmnopqrst", 0);
	test(*r->s3, 0, 0, "", 0);
	test(*r->s3, 0, 0, "abcde", -5);
	test(*r->s3, 0, 0, "abcdefghij", -10);
	test(*r->s3, 0, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s3, 0, 1, "", 1);
	test(*r->s3, 0, 1, "abcde", -4);
	test(*r->s3, 0, 1, "abcdefghij", -9);
	test(*r->s3, 0, 1, "abcdefghijklmnopqrst", -19);
	test(*r->s3, 0, 5, "", 5);
	test(*r->s3, 0, 5, "abcde", 0);
	test(*r->s3, 0, 5, "abcdefghij", -5);
	test(*r->s3, 0, 5, "abcdefghijklmnopqrst", -15);
	test(*r->s3, 0, 9, "", 9);
	test(*r->s3, 0, 9, "abcde", 4);
	test(*r->s3, 0, 9, "abcdefghij", -1);
	test(*r->s3, 0, 9, "abcdefghijklmnopqrst", -11);
	test(*r->s3, 0, 10, "", 10);
	test(*r->s3, 0, 10, "abcde", 5);
	test(*r->s3, 0, 10, "abcdefghij", 0);
	test(*r->s3, 0, 10, "abcdefghijklmnopqrst", -10);
	test(*r->s3, 0, 11, "", 10);
	test(*r->s3, 0, 11, "abcde", 5);
	test(*r->s3, 0, 11, "abcdefghij", 0);
	test(*r->s3, 0, 11, "abcdefghijklmnopqrst", -10);
	test(*r->s3, 1, 0, "", 0);
	test(*r->s3, 1, 0, "abcde", -5);
	test(*r->s3, 1, 0, "abcdefghij", -10);
	test(*r->s3, 1, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s3, 1, 1, "", 1);
	test(*r->s3, 1, 1, "abcde", 1);
	test(*r->s3, 1, 1, "abcdefghij", 1);
	test(*r->s3, 1, 1, "abcdefghijklmnopqrst", 1);
	test(*r->s3, 1, 4, "", 4);
	test(*r->s3, 1, 4, "abcde", 1);
	test(*r->s3, 1, 4, "abcdefghij", 1);
	test(*r->s3, 1, 4, "abcdefghijklmnopqrst", 1);
	test(*r->s3, 1, 8, "", 8);
	test(*r->s3, 1, 8, "abcde", 1);
	test(*r->s3, 1, 8, "abcdefghij", 1);
	test(*r->s3, 1, 8, "abcdefghijklmnopqrst", 1);
	test(*r->s3, 1, 9, "", 9);
	test(*r->s3, 1, 9, "abcde", 1);
	test(*r->s3, 1, 9, "abcdefghij", 1);
	test(*r->s3, 1, 9, "abcdefghijklmnopqrst", 1);
	test(*r->s3, 1, 10, "", 9);
	test(*r->s3, 1, 10, "abcde", 1);
	test(*r->s3, 1, 10, "abcdefghij", 1);
	test(*r->s3, 1, 10, "abcdefghijklmnopqrst", 1);
	test(*r->s3, 5, 0, "", 0);
	test(*r->s3, 5, 0, "abcde", -5);
	test(*r->s3, 5, 0, "abcdefghij", -10);
	test(*r->s3, 5, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s3, 5, 1, "", 1);
	test(*r->s3, 5, 1, "abcde", 5);
	test(*r->s3, 5, 1, "abcdefghij", 5);
	test(*r->s3, 5, 1, "abcdefghijklmnopqrst", 5);
	test(*r->s3, 5, 2, "", 2);
	test(*r->s3, 5, 2, "abcde", 5);
	test(*r->s3, 5, 2, "abcdefghij", 5);
	test(*r->s3, 5, 2, "abcdefghijklmnopqrst", 5);
	test(*r->s3, 5, 4, "", 4);
	test(*r->s3, 5, 4, "abcde", 5);
	test(*r->s3, 5, 4, "abcdefghij", 5);
	test(*r->s3, 5, 4, "abcdefghijklmnopqrst", 5);
	test(*r->s3, 5, 5, "", 5);
	test(*r->s3, 5, 5, "abcde", 5);
	test(*r->s3, 5, 5, "abcdefghij", 5);
	test(*r->s3, 5, 5, "abcdefghijklmnopqrst", 5);
	test(*r->s3, 5, 6, "", 5);
	test(*r->s3, 5, 6, "abcde", 5);
	test(*r->s3, 5, 6, "abcdefghij", 5);
	test(*r->s3, 5, 6, "abcdefghijklmnopqrst", 5);
	test(*r->s3, 9, 0, "", 0);
	test(*r->s3, 9, 0, "abcde", -5);
	test(*r->s3, 9, 0, "abcdefghij", -10);
	test(*r->s3, 9, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s3, 9, 1, "", 1);
	test(*r->s3, 9, 1, "abcde", 9);
	test(*r->s3, 9, 1, "abcdefghij", 9);
	test(*r->s3, 9, 1, "abcdefghijklmnopqrst", 9);
	test(*r->s3, 9, 2, "", 1);
	test(*r->s3, 9, 2, "abcde", 9);
	test(*r->s3, 9, 2, "abcdefghij", 9);
	test(*r->s3, 9, 2, "abcdefghijklmnopqrst", 9);
	test(*r->s3, 10, 0, "", 0);
	test(*r->s3, 10, 0, "abcde", -5);
	test(*r->s3, 10, 0, "abcdefghij", -10);
	test(*r->s3, 10, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s3, 10, 1, "", 0);
	test(*r->s3, 10, 1, "abcde", -5);
	test(*r->s3, 10, 1, "abcdefghij", -10);
	test(*r->s3, 10, 1, "abcdefghijklmnopqrst", -20);
	test(*r->s3, 11, 0, "", 0);
	test(*r->s3, 11, 0, "abcde", 0);
	test(*r->s3, 11, 0, "abcdefghij", 0);
	test(*r->s3, 11, 0, "abcdefghijklmnopqrst", 0);
}

template <class S>
void
test2(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	test(*r->s4, 0, 0, "", 0);
	test(*r->s4, 0, 0, "abcde", -5);
	test(*r->s4, 0, 0, "abcdefghij", -10);
	test(*r->s4, 0, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s4, 0, 1, "", 1);
	test(*r->s4, 0, 1, "abcde", -4);
	test(*r->s4, 0, 1, "abcdefghij", -9);
	test(*r->s4, 0, 1, "abcdefghijklmnopqrst", -19);
	test(*r->s4, 0, 10, "", 10);
	test(*r->s4, 0, 10, "abcde", 5);
	test(*r->s4, 0, 10, "abcdefghij", 0);
	test(*r->s4, 0, 10, "abcdefghijklmnopqrst", -10);
	test(*r->s4, 0, 19, "", 19);
	test(*r->s4, 0, 19, "abcde", 14);
	test(*r->s4, 0, 19, "abcdefghij", 9);
	test(*r->s4, 0, 19, "abcdefghijklmnopqrst", -1);
	test(*r->s4, 0, 20, "", 20);
	test(*r->s4, 0, 20, "abcde", 15);
	test(*r->s4, 0, 20, "abcdefghij", 10);
	test(*r->s4, 0, 20, "abcdefghijklmnopqrst", 0);
	test(*r->s4, 0, 21, "", 20);
	test(*r->s4, 0, 21, "abcde", 15);
	test(*r->s4, 0, 21, "abcdefghij", 10);
	test(*r->s4, 0, 21, "abcdefghijklmnopqrst", 0);
	test(*r->s4, 1, 0, "", 0);
	test(*r->s4, 1, 0, "abcde", -5);
	test(*r->s4, 1, 0, "abcdefghij", -10);
	test(*r->s4, 1, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s4, 1, 1, "", 1);
	test(*r->s4, 1, 1, "abcde", 1);
	test(*r->s4, 1, 1, "abcdefghij", 1);
	test(*r->s4, 1, 1, "abcdefghijklmnopqrst", 1);
	test(*r->s4, 1, 9, "", 9);
	test(*r->s4, 1, 9, "abcde", 1);
	test(*r->s4, 1, 9, "abcdefghij", 1);
	test(*r->s4, 1, 9, "abcdefghijklmnopqrst", 1);
	test(*r->s4, 1, 18, "", 18);
	test(*r->s4, 1, 18, "abcde", 1);
	test(*r->s4, 1, 18, "abcdefghij", 1);
	test(*r->s4, 1, 18, "abcdefghijklmnopqrst", 1);
	test(*r->s4, 1, 19, "", 19);
	test(*r->s4, 1, 19, "abcde", 1);
	test(*r->s4, 1, 19, "abcdefghij", 1);
	test(*r->s4, 1, 19, "abcdefghijklmnopqrst", 1);
	test(*r->s4, 1, 20, "", 19);
	test(*r->s4, 1, 20, "abcde", 1);
	test(*r->s4, 1, 20, "abcdefghij", 1);
	test(*r->s4, 1, 20, "abcdefghijklmnopqrst", 1);
	test(*r->s4, 10, 0, "", 0);
	test(*r->s4, 10, 0, "abcde", -5);
	test(*r->s4, 10, 0, "abcdefghij", -10);
	test(*r->s4, 10, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s4, 10, 1, "", 1);
	test(*r->s4, 10, 1, "abcde", 10);
	test(*r->s4, 10, 1, "abcdefghij", 10);
	test(*r->s4, 10, 1, "abcdefghijklmnopqrst", 10);
	test(*r->s4, 10, 5, "", 5);
	test(*r->s4, 10, 5, "abcde", 10);
	test(*r->s4, 10, 5, "abcdefghij", 10);
	test(*r->s4, 10, 5, "abcdefghijklmnopqrst", 10);
	test(*r->s4, 10, 9, "", 9);
	test(*r->s4, 10, 9, "abcde", 10);
	test(*r->s4, 10, 9, "abcdefghij", 10);
	test(*r->s4, 10, 9, "abcdefghijklmnopqrst", 10);
	test(*r->s4, 10, 10, "", 10);
	test(*r->s4, 10, 10, "abcde", 10);
	test(*r->s4, 10, 10, "abcdefghij", 10);
	test(*r->s4, 10, 10, "abcdefghijklmnopqrst", 10);
	test(*r->s4, 10, 11, "", 10);
	test(*r->s4, 10, 11, "abcde", 10);
	test(*r->s4, 10, 11, "abcdefghij", 10);
	test(*r->s4, 10, 11, "abcdefghijklmnopqrst", 10);
	test(*r->s4, 19, 0, "", 0);
	test(*r->s4, 19, 0, "abcde", -5);
	test(*r->s4, 19, 0, "abcdefghij", -10);
	test(*r->s4, 19, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s4, 19, 1, "", 1);
	test(*r->s4, 19, 1, "abcde", 19);
	test(*r->s4, 19, 1, "abcdefghij", 19);
	test(*r->s4, 19, 1, "abcdefghijklmnopqrst", 19);
	test(*r->s4, 19, 2, "", 1);
	test(*r->s4, 19, 2, "abcde", 19);
	test(*r->s4, 19, 2, "abcdefghij", 19);
	test(*r->s4, 19, 2, "abcdefghijklmnopqrst", 19);
	test(*r->s4, 20, 0, "", 0);
	test(*r->s4, 20, 0, "abcde", -5);
	test(*r->s4, 20, 0, "abcdefghij", -10);
	test(*r->s4, 20, 0, "abcdefghijklmnopqrst", -20);
	test(*r->s4, 20, 1, "", 0);
	test(*r->s4, 20, 1, "abcde", -5);
	test(*r->s4, 20, 1, "abcdefghij", -10);
	test(*r->s4, 20, 1, "abcdefghijklmnopqrst", -20);
	test(*r->s4, 21, 0, "", 0);
	test(*r->s4, 21, 0, "abcde", 0);
	test(*r->s4, 21, 0, "abcdefghij", 0);
	test(*r->s4, 21, 0, "abcdefghijklmnopqrst", 0);
}

void
run(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<pmem::obj::string>("");
			r->s2 = nvobj::make_persistent<pmem::obj::string>(
				"abcde");
			r->s3 = nvobj::make_persistent<pmem::obj::string>(
				"abcdefghij");
			r->s4 = nvobj::make_persistent<pmem::obj::string>(
				"abcdefghijklmnopqrst");
		});

		using S = pmem::obj::string;
		test0<S>(pop);
		test1<S>(pop);
		test2<S>(pop);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem::obj::string>(r->s1);
			nvobj::delete_persistent<pmem::obj::string>(r->s2);
			nvobj::delete_persistent<pmem::obj::string>(r->s3);
			nvobj::delete_persistent<pmem::obj::string>(r->s4);
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
