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

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s_arr[4];
};

template <class S>
void
test(S &s1, typename S::value_type s2)
{
	typedef typename S::traits_type T;
	s1 = s2;
	UT_ASSERT(s1.size() == 1);
	UT_ASSERT(T::eq(s1[0], s2));
	UT_ASSERT(s1.capacity() >= s1.size());
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
			s_arr[2] = nvobj::make_persistent<S>("123456789");
			s_arr[3] = nvobj::make_persistent<S>(
				"1234567890123456789012345678901234567890123456789012345678901234567890");
		});

		test(*s_arr[0], 'a');
		test(*s_arr[1], 'a');
		test(*s_arr[2], 'a');
		test(*s_arr[3], 'a');

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 4; ++i) {
				nvobj::delete_persistent<S>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
