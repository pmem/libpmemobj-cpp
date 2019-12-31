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

#include <array>
#include <libpmemobj++/container/string.hpp>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s1, s2, s3, s4;
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

template <class S, class U>
void
test(const S &s, typename S::size_type pos1, typename S::size_type n1,
     const U &str, int x)
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

template <class S, class U>
void
test0(pmem::obj::persistent_ptr<root> &r, const std::array<U, 4> &arr)
{
	test(*r->s1, 0, 0, *arr[0], 0);
	test(*r->s1, 0, 0, *arr[1], -5);
	test(*r->s1, 0, 0, *arr[2], -10);
	test(*r->s1, 0, 0, *arr[3], -20);
	test(*r->s1, 0, 1, *arr[0], 0);
	test(*r->s1, 0, 1, *arr[1], -5);
	test(*r->s1, 0, 1, *arr[2], -10);
	test(*r->s1, 0, 1, *arr[3], -20);
	test(*r->s1, 1, 0, *arr[0], 0);
	test(*r->s1, 1, 0, *arr[1], 0);
	test(*r->s1, 1, 0, *arr[2], 0);
	test(*r->s1, 1, 0, *arr[3], 0);
	test(*r->s2, 0, 0, *arr[0], 0);
	test(*r->s2, 0, 0, *arr[1], -5);
	test(*r->s2, 0, 0, *arr[2], -10);
	test(*r->s2, 0, 0, *arr[3], -20);
	test(*r->s2, 0, 1, *arr[0], 1);
	test(*r->s2, 0, 1, *arr[1], -4);
	test(*r->s2, 0, 1, *arr[2], -9);
	test(*r->s2, 0, 1, *arr[3], -19);
	test(*r->s2, 0, 2, *arr[0], 2);
	test(*r->s2, 0, 2, *arr[1], -3);
	test(*r->s2, 0, 2, *arr[2], -8);
	test(*r->s2, 0, 2, *arr[3], -18);
	test(*r->s2, 0, 4, *arr[0], 4);
	test(*r->s2, 0, 4, *arr[1], -1);
	test(*r->s2, 0, 4, *arr[2], -6);
	test(*r->s2, 0, 4, *arr[3], -16);
	test(*r->s2, 0, 5, *arr[0], 5);
	test(*r->s2, 0, 5, *arr[1], 0);
	test(*r->s2, 0, 5, *arr[2], -5);
	test(*r->s2, 0, 5, *arr[3], -15);
	test(*r->s2, 0, 6, *arr[0], 5);
	test(*r->s2, 0, 6, *arr[1], 0);
	test(*r->s2, 0, 6, *arr[2], -5);
	test(*r->s2, 0, 6, *arr[3], -15);
	test(*r->s2, 1, 0, *arr[0], 0);
	test(*r->s2, 1, 0, *arr[1], -5);
	test(*r->s2, 1, 0, *arr[2], -10);
	test(*r->s2, 1, 0, *arr[3], -20);
	test(*r->s2, 1, 1, *arr[0], 1);
	test(*r->s2, 1, 1, *arr[1], 1);
	test(*r->s2, 1, 1, *arr[2], 1);
	test(*r->s2, 1, 1, *arr[3], 1);
	test(*r->s2, 1, 2, *arr[0], 2);
	test(*r->s2, 1, 2, *arr[1], 1);
	test(*r->s2, 1, 2, *arr[2], 1);
	test(*r->s2, 1, 2, *arr[3], 1);
	test(*r->s2, 1, 3, *arr[0], 3);
	test(*r->s2, 1, 3, *arr[1], 1);
	test(*r->s2, 1, 3, *arr[2], 1);
	test(*r->s2, 1, 3, *arr[3], 1);
	test(*r->s2, 1, 4, *arr[0], 4);
	test(*r->s2, 1, 4, *arr[1], 1);
	test(*r->s2, 1, 4, *arr[2], 1);
	test(*r->s2, 1, 4, *arr[3], 1);
	test(*r->s2, 1, 5, *arr[0], 4);
	test(*r->s2, 1, 5, *arr[1], 1);
	test(*r->s2, 1, 5, *arr[2], 1);
	test(*r->s2, 1, 5, *arr[3], 1);
	test(*r->s2, 2, 0, *arr[0], 0);
	test(*r->s2, 2, 0, *arr[1], -5);
	test(*r->s2, 2, 0, *arr[2], -10);
	test(*r->s2, 2, 0, *arr[3], -20);
	test(*r->s2, 2, 1, *arr[0], 1);
	test(*r->s2, 2, 1, *arr[1], 2);
	test(*r->s2, 2, 1, *arr[2], 2);
	test(*r->s2, 2, 1, *arr[3], 2);
	test(*r->s2, 2, 2, *arr[0], 2);
	test(*r->s2, 2, 2, *arr[1], 2);
	test(*r->s2, 2, 2, *arr[2], 2);
	test(*r->s2, 2, 2, *arr[3], 2);
	test(*r->s2, 2, 3, *arr[0], 3);
	test(*r->s2, 2, 3, *arr[1], 2);
	test(*r->s2, 2, 3, *arr[2], 2);
	test(*r->s2, 2, 3, *arr[3], 2);
	test(*r->s2, 2, 4, *arr[0], 3);
	test(*r->s2, 2, 4, *arr[1], 2);
	test(*r->s2, 2, 4, *arr[2], 2);
	test(*r->s2, 2, 4, *arr[3], 2);
	test(*r->s2, 4, 0, *arr[0], 0);
	test(*r->s2, 4, 0, *arr[1], -5);
	test(*r->s2, 4, 0, *arr[2], -10);
	test(*r->s2, 4, 0, *arr[3], -20);
	test(*r->s2, 4, 1, *arr[0], 1);
	test(*r->s2, 4, 1, *arr[1], 4);
	test(*r->s2, 4, 1, *arr[2], 4);
	test(*r->s2, 4, 1, *arr[3], 4);
	test(*r->s2, 4, 2, *arr[0], 1);
	test(*r->s2, 4, 2, *arr[1], 4);
	test(*r->s2, 4, 2, *arr[2], 4);
	test(*r->s2, 4, 2, *arr[3], 4);
	test(*r->s2, 5, 0, *arr[0], 0);
	test(*r->s2, 5, 0, *arr[1], -5);
	test(*r->s2, 5, 0, *arr[2], -10);
	test(*r->s2, 5, 0, *arr[3], -20);
	test(*r->s2, 5, 1, *arr[0], 0);
	test(*r->s2, 5, 1, *arr[1], -5);
	test(*r->s2, 5, 1, *arr[2], -10);
	test(*r->s2, 5, 1, *arr[3], -20);
}

template <class S, class U>
void
test1(pmem::obj::persistent_ptr<root> &r, const std::array<U, 4> &arr)
{
	test(*r->s2, 6, 0, *arr[0], 0);
	test(*r->s2, 6, 0, *arr[1], 0);
	test(*r->s2, 6, 0, *arr[2], 0);
	test(*r->s2, 6, 0, *arr[3], 0);
	test(*r->s3, 0, 0, *arr[0], 0);
	test(*r->s3, 0, 0, *arr[1], -5);
	test(*r->s3, 0, 0, *arr[2], -10);
	test(*r->s3, 0, 0, *arr[3], -20);
	test(*r->s3, 0, 1, *arr[0], 1);
	test(*r->s3, 0, 1, *arr[1], -4);
	test(*r->s3, 0, 1, *arr[2], -9);
	test(*r->s3, 0, 1, *arr[3], -19);
	test(*r->s3, 0, 5, *arr[0], 5);
	test(*r->s3, 0, 5, *arr[1], 0);
	test(*r->s3, 0, 5, *arr[2], -5);
	test(*r->s3, 0, 5, *arr[3], -15);
	test(*r->s3, 0, 9, *arr[0], 9);
	test(*r->s3, 0, 9, *arr[1], 4);
	test(*r->s3, 0, 9, *arr[2], -1);
	test(*r->s3, 0, 9, *arr[3], -11);
	test(*r->s3, 0, 10, *arr[0], 10);
	test(*r->s3, 0, 10, *arr[1], 5);
	test(*r->s3, 0, 10, *arr[2], 0);
	test(*r->s3, 0, 10, *arr[3], -10);
	test(*r->s3, 0, 11, *arr[0], 10);
	test(*r->s3, 0, 11, *arr[1], 5);
	test(*r->s3, 0, 11, *arr[2], 0);
	test(*r->s3, 0, 11, *arr[3], -10);
	test(*r->s3, 1, 0, *arr[0], 0);
	test(*r->s3, 1, 0, *arr[1], -5);
	test(*r->s3, 1, 0, *arr[2], -10);
	test(*r->s3, 1, 0, *arr[3], -20);
	test(*r->s3, 1, 1, *arr[0], 1);
	test(*r->s3, 1, 1, *arr[1], 1);
	test(*r->s3, 1, 1, *arr[2], 1);
	test(*r->s3, 1, 1, *arr[3], 1);
	test(*r->s3, 1, 4, *arr[0], 4);
	test(*r->s3, 1, 4, *arr[1], 1);
	test(*r->s3, 1, 4, *arr[2], 1);
	test(*r->s3, 1, 4, *arr[3], 1);
	test(*r->s3, 1, 8, *arr[0], 8);
	test(*r->s3, 1, 8, *arr[1], 1);
	test(*r->s3, 1, 8, *arr[2], 1);
	test(*r->s3, 1, 8, *arr[3], 1);
	test(*r->s3, 1, 9, *arr[0], 9);
	test(*r->s3, 1, 9, *arr[1], 1);
	test(*r->s3, 1, 9, *arr[2], 1);
	test(*r->s3, 1, 9, *arr[3], 1);
	test(*r->s3, 1, 10, *arr[0], 9);
	test(*r->s3, 1, 10, *arr[1], 1);
	test(*r->s3, 1, 10, *arr[2], 1);
	test(*r->s3, 1, 10, *arr[3], 1);
	test(*r->s3, 5, 0, *arr[0], 0);
	test(*r->s3, 5, 0, *arr[1], -5);
	test(*r->s3, 5, 0, *arr[2], -10);
	test(*r->s3, 5, 0, *arr[3], -20);
	test(*r->s3, 5, 1, *arr[0], 1);
	test(*r->s3, 5, 1, *arr[1], 5);
	test(*r->s3, 5, 1, *arr[2], 5);
	test(*r->s3, 5, 1, *arr[3], 5);
	test(*r->s3, 5, 2, *arr[0], 2);
	test(*r->s3, 5, 2, *arr[1], 5);
	test(*r->s3, 5, 2, *arr[2], 5);
	test(*r->s3, 5, 2, *arr[3], 5);
	test(*r->s3, 5, 4, *arr[0], 4);
	test(*r->s3, 5, 4, *arr[1], 5);
	test(*r->s3, 5, 4, *arr[2], 5);
	test(*r->s3, 5, 4, *arr[3], 5);
	test(*r->s3, 5, 5, *arr[0], 5);
	test(*r->s3, 5, 5, *arr[1], 5);
	test(*r->s3, 5, 5, *arr[2], 5);
	test(*r->s3, 5, 5, *arr[3], 5);
	test(*r->s3, 5, 6, *arr[0], 5);
	test(*r->s3, 5, 6, *arr[1], 5);
	test(*r->s3, 5, 6, *arr[2], 5);
	test(*r->s3, 5, 6, *arr[3], 5);
	test(*r->s3, 9, 0, *arr[0], 0);
	test(*r->s3, 9, 0, *arr[1], -5);
	test(*r->s3, 9, 0, *arr[2], -10);
	test(*r->s3, 9, 0, *arr[3], -20);
	test(*r->s3, 9, 1, *arr[0], 1);
	test(*r->s3, 9, 1, *arr[1], 9);
	test(*r->s3, 9, 1, *arr[2], 9);
	test(*r->s3, 9, 1, *arr[3], 9);
	test(*r->s3, 9, 2, *arr[0], 1);
	test(*r->s3, 9, 2, *arr[1], 9);
	test(*r->s3, 9, 2, *arr[2], 9);
	test(*r->s3, 9, 2, *arr[3], 9);
	test(*r->s3, 10, 0, *arr[0], 0);
	test(*r->s3, 10, 0, *arr[1], -5);
	test(*r->s3, 10, 0, *arr[2], -10);
	test(*r->s3, 10, 0, *arr[3], -20);
	test(*r->s3, 10, 1, *arr[0], 0);
	test(*r->s3, 10, 1, *arr[1], -5);
	test(*r->s3, 10, 1, *arr[2], -10);
	test(*r->s3, 10, 1, *arr[3], -20);
	test(*r->s3, 11, 0, *arr[0], 0);
	test(*r->s3, 11, 0, *arr[1], 0);
	test(*r->s3, 11, 0, *arr[2], 0);
	test(*r->s3, 11, 0, *arr[3], 0);
}

template <class S, class U>
void
test2(pmem::obj::persistent_ptr<root> &r, const std::array<U, 4> &arr)
{
	test(*r->s4, 0, 0, *arr[0], 0);
	test(*r->s4, 0, 0, *arr[1], -5);
	test(*r->s4, 0, 0, *arr[2], -10);
	test(*r->s4, 0, 0, *arr[3], -20);
	test(*r->s4, 0, 1, *arr[0], 1);
	test(*r->s4, 0, 1, *arr[1], -4);
	test(*r->s4, 0, 1, *arr[2], -9);
	test(*r->s4, 0, 1, *arr[3], -19);
	test(*r->s4, 0, 10, *arr[0], 10);
	test(*r->s4, 0, 10, *arr[1], 5);
	test(*r->s4, 0, 10, *arr[2], 0);
	test(*r->s4, 0, 10, *arr[3], -10);
	test(*r->s4, 0, 19, *arr[0], 19);
	test(*r->s4, 0, 19, *arr[1], 14);
	test(*r->s4, 0, 19, *arr[2], 9);
	test(*r->s4, 0, 19, *arr[3], -1);
	test(*r->s4, 0, 20, *arr[0], 20);
	test(*r->s4, 0, 20, *arr[1], 15);
	test(*r->s4, 0, 20, *arr[2], 10);
	test(*r->s4, 0, 20, *arr[3], 0);
	test(*r->s4, 0, 21, *arr[0], 20);
	test(*r->s4, 0, 21, *arr[1], 15);
	test(*r->s4, 0, 21, *arr[2], 10);
	test(*r->s4, 0, 21, *arr[3], 0);
	test(*r->s4, 1, 0, *arr[0], 0);
	test(*r->s4, 1, 0, *arr[1], -5);
	test(*r->s4, 1, 0, *arr[2], -10);
	test(*r->s4, 1, 0, *arr[3], -20);
	test(*r->s4, 1, 1, *arr[0], 1);
	test(*r->s4, 1, 1, *arr[1], 1);
	test(*r->s4, 1, 1, *arr[2], 1);
	test(*r->s4, 1, 1, *arr[3], 1);
	test(*r->s4, 1, 9, *arr[0], 9);
	test(*r->s4, 1, 9, *arr[1], 1);
	test(*r->s4, 1, 9, *arr[2], 1);
	test(*r->s4, 1, 9, *arr[3], 1);
	test(*r->s4, 1, 18, *arr[0], 18);
	test(*r->s4, 1, 18, *arr[1], 1);
	test(*r->s4, 1, 18, *arr[2], 1);
	test(*r->s4, 1, 18, *arr[3], 1);
	test(*r->s4, 1, 19, *arr[0], 19);
	test(*r->s4, 1, 19, *arr[1], 1);
	test(*r->s4, 1, 19, *arr[2], 1);
	test(*r->s4, 1, 19, *arr[3], 1);
	test(*r->s4, 1, 20, *arr[0], 19);
	test(*r->s4, 1, 20, *arr[1], 1);
	test(*r->s4, 1, 20, *arr[2], 1);
	test(*r->s4, 1, 20, *arr[3], 1);
	test(*r->s4, 10, 0, *arr[0], 0);
	test(*r->s4, 10, 0, *arr[1], -5);
	test(*r->s4, 10, 0, *arr[2], -10);
	test(*r->s4, 10, 0, *arr[3], -20);
	test(*r->s4, 10, 1, *arr[0], 1);
	test(*r->s4, 10, 1, *arr[1], 10);
	test(*r->s4, 10, 1, *arr[2], 10);
	test(*r->s4, 10, 1, *arr[3], 10);
	test(*r->s4, 10, 5, *arr[0], 5);
	test(*r->s4, 10, 5, *arr[1], 10);
	test(*r->s4, 10, 5, *arr[2], 10);
	test(*r->s4, 10, 5, *arr[3], 10);
	test(*r->s4, 10, 9, *arr[0], 9);
	test(*r->s4, 10, 9, *arr[1], 10);
	test(*r->s4, 10, 9, *arr[2], 10);
	test(*r->s4, 10, 9, *arr[3], 10);
	test(*r->s4, 10, 10, *arr[0], 10);
	test(*r->s4, 10, 10, *arr[1], 10);
	test(*r->s4, 10, 10, *arr[2], 10);
	test(*r->s4, 10, 10, *arr[3], 10);
	test(*r->s4, 10, 11, *arr[0], 10);
	test(*r->s4, 10, 11, *arr[1], 10);
	test(*r->s4, 10, 11, *arr[2], 10);
	test(*r->s4, 10, 11, *arr[3], 10);
	test(*r->s4, 19, 0, *arr[0], 0);
	test(*r->s4, 19, 0, *arr[1], -5);
	test(*r->s4, 19, 0, *arr[2], -10);
	test(*r->s4, 19, 0, *arr[3], -20);
	test(*r->s4, 19, 1, *arr[0], 1);
	test(*r->s4, 19, 1, *arr[1], 19);
	test(*r->s4, 19, 1, *arr[2], 19);
	test(*r->s4, 19, 1, *arr[3], 19);
	test(*r->s4, 19, 2, *arr[0], 1);
	test(*r->s4, 19, 2, *arr[1], 19);
	test(*r->s4, 19, 2, *arr[2], 19);
	test(*r->s4, 19, 2, *arr[3], 19);
	test(*r->s4, 20, 0, *arr[0], 0);
	test(*r->s4, 20, 0, *arr[1], -5);
	test(*r->s4, 20, 0, *arr[2], -10);
	test(*r->s4, 20, 0, *arr[3], -20);
	test(*r->s4, 20, 1, *arr[0], 0);
	test(*r->s4, 20, 1, *arr[1], -5);
	test(*r->s4, 20, 1, *arr[2], -10);
	test(*r->s4, 20, 1, *arr[3], -20);
	test(*r->s4, 21, 0, *arr[0], 0);
	test(*r->s4, 21, 0, *arr[1], 0);
	test(*r->s4, 21, 0, *arr[2], 0);
	test(*r->s4, 21, 0, *arr[3], 0);
}

void
run(pmem::obj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<S>("");
			r->s2 = nvobj::make_persistent<S>("abcde");
			r->s3 = nvobj::make_persistent<S>("abcdefghij");
			r->s4 = nvobj::make_persistent<S>(
				"abcdefghijklmnopqrst");
		});

		std::array<S *, 4> arr{&*r->s1, &*r->s2, &*r->s3, &*r->s4};

		std::string s1(""), s2("abcde"), s3("abcdefghij"),
			s4("abcdefghijklmnopqrst");
		std::array<std::string *, 4> arr_std_str = {&s1, &s2, &s3, &s4};

		/* test pmem::string with pmem::string comparison */
		test0<S>(r, arr);
		test1<S>(r, arr);
		test2<S>(r, arr);

		/* test pmem::string with std::string comparison */
		test0<S>(r, arr_std_str);
		test1<S>(r, arr_std_str);
		test2<S>(r, arr_std_str);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<S>(r->s1);
			nvobj::delete_persistent<S>(r->s2);
			nvobj::delete_persistent<S>(r->s3);
			nvobj::delete_persistent<S>(r->s4);
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
