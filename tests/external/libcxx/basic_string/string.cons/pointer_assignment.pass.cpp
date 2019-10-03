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

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s1;
	nvobj::persistent_ptr<S> s_arr[6];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s_1,
     const typename S::value_type *s2)
{
	auto r = pop.root();

	nvobj::transaction::run(
		pop, [&] { r->s1 = nvobj::make_persistent<S>(s_1); });

	auto &s1 = *r->s1;

	typedef typename S::traits_type T;
	s1 = s2;
	UT_ASSERT(s1.size() == T::length(s2));
	UT_ASSERT(T::compare(s1.data(), s2, s1.size()) == 0);
	UT_ASSERT(s1.capacity() >= s1.size());

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s1); });
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "string_test", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto &s_arr = pop.root()->s_arr;

	try {
		nvobj::transaction::run(pop, [&] {
			s_arr[0] = nvobj::make_persistent<S>();
			s_arr[1] = nvobj::make_persistent<S>("1");
			s_arr[2] = nvobj::make_persistent<S>("2");
			s_arr[3] = nvobj::make_persistent<S>("123456789");
			s_arr[4] = nvobj::make_persistent<S>(
				"1234567890123456789012345678901234567890123456789012345678901234567890");
			s_arr[5] = nvobj::make_persistent<S>(
				"1234567890123456789012345678901234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890123456789012345678901234567890");
		});

		test(pop, *s_arr[0], "");
		test(pop, *s_arr[1], "");
		test(pop, *s_arr[0], "1");
		test(pop, *s_arr[1], "2");
		test(pop, *s_arr[1], "2");

		test(pop, *s_arr[0],
		     "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
		test(pop, *s_arr[3],
		     "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
		test(pop, *s_arr[4],
		     "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
		test(pop, *s_arr[5],
		     "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 6; ++i) {
				nvobj::delete_persistent<S>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
