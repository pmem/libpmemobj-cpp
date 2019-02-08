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
	nvobj::persistent_ptr<pmem_exp::string> s1, s2, s3, s4;
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
     const S &str, int x)
{
	if (pos1 <= s.size())
		UT_ASSERT(sign(s.compare(pos1, n1, str)) == sign(x));
	else {
		try {
			s.compare(pos1, n1, str);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos1 > s.size());
		}
	}
}

template <class S>
void
test0(pmem::obj::persistent_ptr<root> &r)
{
	test(*r->s1, 0, 0, *r->s1, 0);
	test(*r->s1, 0, 0, *r->s2, -5);
	test(*r->s1, 0, 0, *r->s3, -10);
	test(*r->s1, 0, 0, *r->s4, -20);
	test(*r->s1, 0, 1, *r->s1, 0);
	test(*r->s1, 0, 1, *r->s2, -5);
	test(*r->s1, 0, 1, *r->s3, -10);
	test(*r->s1, 0, 1, *r->s4, -20);
	test(*r->s1, 1, 0, *r->s1, 0);
	test(*r->s1, 1, 0, *r->s2, 0);
	test(*r->s1, 1, 0, *r->s3, 0);
	test(*r->s1, 1, 0, *r->s4, 0);
	test(*r->s2, 0, 0, *r->s1, 0);
	test(*r->s2, 0, 0, *r->s2, -5);
	test(*r->s2, 0, 0, *r->s3, -10);
	test(*r->s2, 0, 0, *r->s4, -20);
	test(*r->s2, 0, 1, *r->s1, 1);
	test(*r->s2, 0, 1, *r->s2, -4);
	test(*r->s2, 0, 1, *r->s3, -9);
	test(*r->s2, 0, 1, *r->s4, -19);
	test(*r->s2, 0, 2, *r->s1, 2);
	test(*r->s2, 0, 2, *r->s2, -3);
	test(*r->s2, 0, 2, *r->s3, -8);
	test(*r->s2, 0, 2, *r->s4, -18);
	test(*r->s2, 0, 4, *r->s1, 4);
	test(*r->s2, 0, 4, *r->s2, -1);
	test(*r->s2, 0, 4, *r->s3, -6);
	test(*r->s2, 0, 4, *r->s4, -16);
	test(*r->s2, 0, 5, *r->s1, 5);
	test(*r->s2, 0, 5, *r->s2, 0);
	test(*r->s2, 0, 5, *r->s3, -5);
	test(*r->s2, 0, 5, *r->s4, -15);
	test(*r->s2, 0, 6, *r->s1, 5);
	test(*r->s2, 0, 6, *r->s2, 0);
	test(*r->s2, 0, 6, *r->s3, -5);
	test(*r->s2, 0, 6, *r->s4, -15);
	test(*r->s2, 1, 0, *r->s1, 0);
	test(*r->s2, 1, 0, *r->s2, -5);
	test(*r->s2, 1, 0, *r->s3, -10);
	test(*r->s2, 1, 0, *r->s4, -20);
	test(*r->s2, 1, 1, *r->s1, 1);
	test(*r->s2, 1, 1, *r->s2, 1);
	test(*r->s2, 1, 1, *r->s3, 1);
	test(*r->s2, 1, 1, *r->s4, 1);
	test(*r->s2, 1, 2, *r->s1, 2);
	test(*r->s2, 1, 2, *r->s2, 1);
	test(*r->s2, 1, 2, *r->s3, 1);
	test(*r->s2, 1, 2, *r->s4, 1);
	test(*r->s2, 1, 3, *r->s1, 3);
	test(*r->s2, 1, 3, *r->s2, 1);
	test(*r->s2, 1, 3, *r->s3, 1);
	test(*r->s2, 1, 3, *r->s4, 1);
	test(*r->s2, 1, 4, *r->s1, 4);
	test(*r->s2, 1, 4, *r->s2, 1);
	test(*r->s2, 1, 4, *r->s3, 1);
	test(*r->s2, 1, 4, *r->s4, 1);
	test(*r->s2, 1, 5, *r->s1, 4);
	test(*r->s2, 1, 5, *r->s2, 1);
	test(*r->s2, 1, 5, *r->s3, 1);
	test(*r->s2, 1, 5, *r->s4, 1);
	test(*r->s2, 2, 0, *r->s1, 0);
	test(*r->s2, 2, 0, *r->s2, -5);
	test(*r->s2, 2, 0, *r->s3, -10);
	test(*r->s2, 2, 0, *r->s4, -20);
	test(*r->s2, 2, 1, *r->s1, 1);
	test(*r->s2, 2, 1, *r->s2, 2);
	test(*r->s2, 2, 1, *r->s3, 2);
	test(*r->s2, 2, 1, *r->s4, 2);
	test(*r->s2, 2, 2, *r->s1, 2);
	test(*r->s2, 2, 2, *r->s2, 2);
	test(*r->s2, 2, 2, *r->s3, 2);
	test(*r->s2, 2, 2, *r->s4, 2);
	test(*r->s2, 2, 3, *r->s1, 3);
	test(*r->s2, 2, 3, *r->s2, 2);
	test(*r->s2, 2, 3, *r->s3, 2);
	test(*r->s2, 2, 3, *r->s4, 2);
	test(*r->s2, 2, 4, *r->s1, 3);
	test(*r->s2, 2, 4, *r->s2, 2);
	test(*r->s2, 2, 4, *r->s3, 2);
	test(*r->s2, 2, 4, *r->s4, 2);
	test(*r->s2, 4, 0, *r->s1, 0);
	test(*r->s2, 4, 0, *r->s2, -5);
	test(*r->s2, 4, 0, *r->s3, -10);
	test(*r->s2, 4, 0, *r->s4, -20);
	test(*r->s2, 4, 1, *r->s1, 1);
	test(*r->s2, 4, 1, *r->s2, 4);
	test(*r->s2, 4, 1, *r->s3, 4);
	test(*r->s2, 4, 1, *r->s4, 4);
	test(*r->s2, 4, 2, *r->s1, 1);
	test(*r->s2, 4, 2, *r->s2, 4);
	test(*r->s2, 4, 2, *r->s3, 4);
	test(*r->s2, 4, 2, *r->s4, 4);
	test(*r->s2, 5, 0, *r->s1, 0);
	test(*r->s2, 5, 0, *r->s2, -5);
	test(*r->s2, 5, 0, *r->s3, -10);
	test(*r->s2, 5, 0, *r->s4, -20);
	test(*r->s2, 5, 1, *r->s1, 0);
	test(*r->s2, 5, 1, *r->s2, -5);
	test(*r->s2, 5, 1, *r->s3, -10);
	test(*r->s2, 5, 1, *r->s4, -20);
}

template <class S>
void
test1(pmem::obj::persistent_ptr<root> &r)
{
	test(*r->s2, 6, 0, *r->s1, 0);
	test(*r->s2, 6, 0, *r->s2, 0);
	test(*r->s2, 6, 0, *r->s3, 0);
	test(*r->s2, 6, 0, *r->s4, 0);
	test(*r->s3, 0, 0, *r->s1, 0);
	test(*r->s3, 0, 0, *r->s2, -5);
	test(*r->s3, 0, 0, *r->s3, -10);
	test(*r->s3, 0, 0, *r->s4, -20);
	test(*r->s3, 0, 1, *r->s1, 1);
	test(*r->s3, 0, 1, *r->s2, -4);
	test(*r->s3, 0, 1, *r->s3, -9);
	test(*r->s3, 0, 1, *r->s4, -19);
	test(*r->s3, 0, 5, *r->s1, 5);
	test(*r->s3, 0, 5, *r->s2, 0);
	test(*r->s3, 0, 5, *r->s3, -5);
	test(*r->s3, 0, 5, *r->s4, -15);
	test(*r->s3, 0, 9, *r->s1, 9);
	test(*r->s3, 0, 9, *r->s2, 4);
	test(*r->s3, 0, 9, *r->s3, -1);
	test(*r->s3, 0, 9, *r->s4, -11);
	test(*r->s3, 0, 10, *r->s1, 10);
	test(*r->s3, 0, 10, *r->s2, 5);
	test(*r->s3, 0, 10, *r->s3, 0);
	test(*r->s3, 0, 10, *r->s4, -10);
	test(*r->s3, 0, 11, *r->s1, 10);
	test(*r->s3, 0, 11, *r->s2, 5);
	test(*r->s3, 0, 11, *r->s3, 0);
	test(*r->s3, 0, 11, *r->s4, -10);
	test(*r->s3, 1, 0, *r->s1, 0);
	test(*r->s3, 1, 0, *r->s2, -5);
	test(*r->s3, 1, 0, *r->s3, -10);
	test(*r->s3, 1, 0, *r->s4, -20);
	test(*r->s3, 1, 1, *r->s1, 1);
	test(*r->s3, 1, 1, *r->s2, 1);
	test(*r->s3, 1, 1, *r->s3, 1);
	test(*r->s3, 1, 1, *r->s4, 1);
	test(*r->s3, 1, 4, *r->s1, 4);
	test(*r->s3, 1, 4, *r->s2, 1);
	test(*r->s3, 1, 4, *r->s3, 1);
	test(*r->s3, 1, 4, *r->s4, 1);
	test(*r->s3, 1, 8, *r->s1, 8);
	test(*r->s3, 1, 8, *r->s2, 1);
	test(*r->s3, 1, 8, *r->s3, 1);
	test(*r->s3, 1, 8, *r->s4, 1);
	test(*r->s3, 1, 9, *r->s1, 9);
	test(*r->s3, 1, 9, *r->s2, 1);
	test(*r->s3, 1, 9, *r->s3, 1);
	test(*r->s3, 1, 9, *r->s4, 1);
	test(*r->s3, 1, 10, *r->s1, 9);
	test(*r->s3, 1, 10, *r->s2, 1);
	test(*r->s3, 1, 10, *r->s3, 1);
	test(*r->s3, 1, 10, *r->s4, 1);
	test(*r->s3, 5, 0, *r->s1, 0);
	test(*r->s3, 5, 0, *r->s2, -5);
	test(*r->s3, 5, 0, *r->s3, -10);
	test(*r->s3, 5, 0, *r->s4, -20);
	test(*r->s3, 5, 1, *r->s1, 1);
	test(*r->s3, 5, 1, *r->s2, 5);
	test(*r->s3, 5, 1, *r->s3, 5);
	test(*r->s3, 5, 1, *r->s4, 5);
	test(*r->s3, 5, 2, *r->s1, 2);
	test(*r->s3, 5, 2, *r->s2, 5);
	test(*r->s3, 5, 2, *r->s3, 5);
	test(*r->s3, 5, 2, *r->s4, 5);
	test(*r->s3, 5, 4, *r->s1, 4);
	test(*r->s3, 5, 4, *r->s2, 5);
	test(*r->s3, 5, 4, *r->s3, 5);
	test(*r->s3, 5, 4, *r->s4, 5);
	test(*r->s3, 5, 5, *r->s1, 5);
	test(*r->s3, 5, 5, *r->s2, 5);
	test(*r->s3, 5, 5, *r->s3, 5);
	test(*r->s3, 5, 5, *r->s4, 5);
	test(*r->s3, 5, 6, *r->s1, 5);
	test(*r->s3, 5, 6, *r->s2, 5);
	test(*r->s3, 5, 6, *r->s3, 5);
	test(*r->s3, 5, 6, *r->s4, 5);
	test(*r->s3, 9, 0, *r->s1, 0);
	test(*r->s3, 9, 0, *r->s2, -5);
	test(*r->s3, 9, 0, *r->s3, -10);
	test(*r->s3, 9, 0, *r->s4, -20);
	test(*r->s3, 9, 1, *r->s1, 1);
	test(*r->s3, 9, 1, *r->s2, 9);
	test(*r->s3, 9, 1, *r->s3, 9);
	test(*r->s3, 9, 1, *r->s4, 9);
	test(*r->s3, 9, 2, *r->s1, 1);
	test(*r->s3, 9, 2, *r->s2, 9);
	test(*r->s3, 9, 2, *r->s3, 9);
	test(*r->s3, 9, 2, *r->s4, 9);
	test(*r->s3, 10, 0, *r->s1, 0);
	test(*r->s3, 10, 0, *r->s2, -5);
	test(*r->s3, 10, 0, *r->s3, -10);
	test(*r->s3, 10, 0, *r->s4, -20);
	test(*r->s3, 10, 1, *r->s1, 0);
	test(*r->s3, 10, 1, *r->s2, -5);
	test(*r->s3, 10, 1, *r->s3, -10);
	test(*r->s3, 10, 1, *r->s4, -20);
	test(*r->s3, 11, 0, *r->s1, 0);
	test(*r->s3, 11, 0, *r->s2, 0);
	test(*r->s3, 11, 0, *r->s3, 0);
	test(*r->s3, 11, 0, *r->s4, 0);
}

template <class S>
void
test2(pmem::obj::persistent_ptr<root> &r)
{
	test(*r->s4, 0, 0, *r->s1, 0);
	test(*r->s4, 0, 0, *r->s2, -5);
	test(*r->s4, 0, 0, *r->s3, -10);
	test(*r->s4, 0, 0, *r->s4, -20);
	test(*r->s4, 0, 1, *r->s1, 1);
	test(*r->s4, 0, 1, *r->s2, -4);
	test(*r->s4, 0, 1, *r->s3, -9);
	test(*r->s4, 0, 1, *r->s4, -19);
	test(*r->s4, 0, 10, *r->s1, 10);
	test(*r->s4, 0, 10, *r->s2, 5);
	test(*r->s4, 0, 10, *r->s3, 0);
	test(*r->s4, 0, 10, *r->s4, -10);
	test(*r->s4, 0, 19, *r->s1, 19);
	test(*r->s4, 0, 19, *r->s2, 14);
	test(*r->s4, 0, 19, *r->s3, 9);
	test(*r->s4, 0, 19, *r->s4, -1);
	test(*r->s4, 0, 20, *r->s1, 20);
	test(*r->s4, 0, 20, *r->s2, 15);
	test(*r->s4, 0, 20, *r->s3, 10);
	test(*r->s4, 0, 20, *r->s4, 0);
	test(*r->s4, 0, 21, *r->s1, 20);
	test(*r->s4, 0, 21, *r->s2, 15);
	test(*r->s4, 0, 21, *r->s3, 10);
	test(*r->s4, 0, 21, *r->s4, 0);
	test(*r->s4, 1, 0, *r->s1, 0);
	test(*r->s4, 1, 0, *r->s2, -5);
	test(*r->s4, 1, 0, *r->s3, -10);
	test(*r->s4, 1, 0, *r->s4, -20);
	test(*r->s4, 1, 1, *r->s1, 1);
	test(*r->s4, 1, 1, *r->s2, 1);
	test(*r->s4, 1, 1, *r->s3, 1);
	test(*r->s4, 1, 1, *r->s4, 1);
	test(*r->s4, 1, 9, *r->s1, 9);
	test(*r->s4, 1, 9, *r->s2, 1);
	test(*r->s4, 1, 9, *r->s3, 1);
	test(*r->s4, 1, 9, *r->s4, 1);
	test(*r->s4, 1, 18, *r->s1, 18);
	test(*r->s4, 1, 18, *r->s2, 1);
	test(*r->s4, 1, 18, *r->s3, 1);
	test(*r->s4, 1, 18, *r->s4, 1);
	test(*r->s4, 1, 19, *r->s1, 19);
	test(*r->s4, 1, 19, *r->s2, 1);
	test(*r->s4, 1, 19, *r->s3, 1);
	test(*r->s4, 1, 19, *r->s4, 1);
	test(*r->s4, 1, 20, *r->s1, 19);
	test(*r->s4, 1, 20, *r->s2, 1);
	test(*r->s4, 1, 20, *r->s3, 1);
	test(*r->s4, 1, 20, *r->s4, 1);
	test(*r->s4, 10, 0, *r->s1, 0);
	test(*r->s4, 10, 0, *r->s2, -5);
	test(*r->s4, 10, 0, *r->s3, -10);
	test(*r->s4, 10, 0, *r->s4, -20);
	test(*r->s4, 10, 1, *r->s1, 1);
	test(*r->s4, 10, 1, *r->s2, 10);
	test(*r->s4, 10, 1, *r->s3, 10);
	test(*r->s4, 10, 1, *r->s4, 10);
	test(*r->s4, 10, 5, *r->s1, 5);
	test(*r->s4, 10, 5, *r->s2, 10);
	test(*r->s4, 10, 5, *r->s3, 10);
	test(*r->s4, 10, 5, *r->s4, 10);
	test(*r->s4, 10, 9, *r->s1, 9);
	test(*r->s4, 10, 9, *r->s2, 10);
	test(*r->s4, 10, 9, *r->s3, 10);
	test(*r->s4, 10, 9, *r->s4, 10);
	test(*r->s4, 10, 10, *r->s1, 10);
	test(*r->s4, 10, 10, *r->s2, 10);
	test(*r->s4, 10, 10, *r->s3, 10);
	test(*r->s4, 10, 10, *r->s4, 10);
	test(*r->s4, 10, 11, *r->s1, 10);
	test(*r->s4, 10, 11, *r->s2, 10);
	test(*r->s4, 10, 11, *r->s3, 10);
	test(*r->s4, 10, 11, *r->s4, 10);
	test(*r->s4, 19, 0, *r->s1, 0);
	test(*r->s4, 19, 0, *r->s2, -5);
	test(*r->s4, 19, 0, *r->s3, -10);
	test(*r->s4, 19, 0, *r->s4, -20);
	test(*r->s4, 19, 1, *r->s1, 1);
	test(*r->s4, 19, 1, *r->s2, 19);
	test(*r->s4, 19, 1, *r->s3, 19);
	test(*r->s4, 19, 1, *r->s4, 19);
	test(*r->s4, 19, 2, *r->s1, 1);
	test(*r->s4, 19, 2, *r->s2, 19);
	test(*r->s4, 19, 2, *r->s3, 19);
	test(*r->s4, 19, 2, *r->s4, 19);
	test(*r->s4, 20, 0, *r->s1, 0);
	test(*r->s4, 20, 0, *r->s2, -5);
	test(*r->s4, 20, 0, *r->s3, -10);
	test(*r->s4, 20, 0, *r->s4, -20);
	test(*r->s4, 20, 1, *r->s1, 0);
	test(*r->s4, 20, 1, *r->s2, -5);
	test(*r->s4, 20, 1, *r->s3, -10);
	test(*r->s4, 20, 1, *r->s4, -20);
	test(*r->s4, 21, 0, *r->s1, 0);
	test(*r->s4, 21, 0, *r->s2, 0);
	test(*r->s4, 21, 0, *r->s3, 0);
	test(*r->s4, 21, 0, *r->s4, 0);
}

void
run(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<pmem_exp::string>("");
			r->s2 = nvobj::make_persistent<pmem_exp::string>(
				"abcde");
			r->s3 = nvobj::make_persistent<pmem_exp::string>(
				"abcdefghij");
			r->s4 = nvobj::make_persistent<pmem_exp::string>(
				"abcdefghijklmnopqrst");
		});

		{
			typedef pmem_exp::string S;
			test0<S>(r);
			test1<S>(r);
			test2<S>(r);
		}

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<pmem_exp::string>(r->s1);
			nvobj::delete_persistent<pmem_exp::string>(r->s2);
			nvobj::delete_persistent<pmem_exp::string>(r->s3);
			nvobj::delete_persistent<pmem_exp::string>(r->s4);
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
