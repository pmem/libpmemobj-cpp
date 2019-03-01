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

namespace nvobj = pmem::obj;
namespace pmem_exp = pmem::obj::experimental;
using S = pmem_exp::string;

struct root {
	nvobj::persistent_ptr<S> s0, s1, s2;
	nvobj::persistent_ptr<S> s_arr[7];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s_1, const S &s_2)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->s0 = nvobj::make_persistent<S>(s_2);
		r->s1 = nvobj::make_persistent<S>(s_1);
		r->s2 = nvobj::make_persistent<S>(s_2);
	});

	auto &s0 = *r->s0;
	auto &s1 = *r->s1;
	auto &s2 = *r->s2;

	s1 = std::move(s2);
	UT_ASSERT(s1 == s0);
	UT_ASSERT(s1.capacity() >= s1.size());

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<S>(r->s0);
		nvobj::delete_persistent<S>(r->s1);
		nvobj::delete_persistent<S>(r->s2);
	});
}

void
test_self_assignment(nvobj::pool<struct root> &pop, const S &s1)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s0 = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s0;
	auto &s_alias = *r->s0;

	s = std::move(s_alias);
	UT_ASSERT(s == s1);
	UT_ASSERT(s.capacity() >= s1.size());

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s0); });
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
			s_arr[6] = nvobj::make_persistent<S>(
				"abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
		});

		test(pop, *s_arr[0], *s_arr[0]);
		test(pop, *s_arr[1], *s_arr[0]);
		test(pop, *s_arr[0], *s_arr[1]);
		test(pop, *s_arr[1], *s_arr[2]);
		test(pop, *s_arr[1], *s_arr[2]);

		test(pop, *s_arr[0], *s_arr[6]);
		test(pop, *s_arr[3], *s_arr[6]);
		test(pop, *s_arr[4], *s_arr[6]);
		test(pop, *s_arr[5], *s_arr[6]);

		test_self_assignment(pop, *s_arr[0]);
		test_self_assignment(pop, *s_arr[3]);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 7; ++i) {
				nvobj::delete_persistent<S>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
