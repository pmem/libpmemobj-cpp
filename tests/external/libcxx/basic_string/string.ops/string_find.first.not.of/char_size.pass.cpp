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
using C = nvobj::string;

struct root {
	nvobj::persistent_ptr<C> s_arr[24];
};

template <class S>
void
test(const S &s, typename S::value_type c, typename S::size_type pos,
     typename S::size_type x)
{
	UT_ASSERT(s.find_first_not_of(c, pos) == x);
	if (x != S::npos)
		UT_ASSERT(pos <= x && x < s.size());
}

template <class S>
void
test(const S &s, typename S::value_type c, typename S::size_type x)
{
	UT_ASSERT(s.find_first_not_of(c) == x);
	if (x != S::npos)
		UT_ASSERT(x < s.size());
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
			s_arr[0] = nvobj::make_persistent<C>("");
			s_arr[1] = nvobj::make_persistent<C>(
				"bnrpehidofmqtcksjgla");
			s_arr[2] = nvobj::make_persistent<C>("csope");
			s_arr[3] = nvobj::make_persistent<C>("eolhfgpjqk");
			s_arr[4] = nvobj::make_persistent<C>("gfsmthlkon");
			s_arr[5] = nvobj::make_persistent<C>("gmfhdaipsr");
			s_arr[6] = nvobj::make_persistent<C>(
				"hkbgspofltajcnedqmri");
			s_arr[7] = nvobj::make_persistent<C>("irkhs");
			s_arr[8] = nvobj::make_persistent<C>(
				"jdmciepkaqgotsrfnhlb");
			s_arr[9] = nvobj::make_persistent<C>(
				"jtdaefblsokrmhpgcnqi");
			s_arr[10] = nvobj::make_persistent<C>("kantesmpgj");
			s_arr[11] = nvobj::make_persistent<C>("kitcj");
			s_arr[12] = nvobj::make_persistent<C>(
				"laenfsbridchgotmkqpj");
			s_arr[13] = nvobj::make_persistent<C>("lahfb");
			s_arr[14] = nvobj::make_persistent<C>(
				"nbatdlmekrgcfqsophij");
			s_arr[15] = nvobj::make_persistent<C>("nhmko");
			s_arr[16] = nvobj::make_persistent<C>("odaftiegpm");
			s_arr[17] = nvobj::make_persistent<C>("oknlrstdpi");
			s_arr[18] = nvobj::make_persistent<C>(
				"oselktgbcapndfjihrmq");
			s_arr[19] = nvobj::make_persistent<C>("pcdrofikas");
			s_arr[20] = nvobj::make_persistent<C>("q");
			s_arr[21] = nvobj::make_persistent<C>("qkamf");
			s_arr[22] = nvobj::make_persistent<C>("qqq");
			s_arr[23] = nvobj::make_persistent<C>("tpsaf");
		});

		test(*s_arr[0], 'q', 0, C::npos);
		test(*s_arr[0], 'q', 1, C::npos);
		test(*s_arr[11], 'q', 0, 0);
		test(*s_arr[21], 'q', 1, 1);
		test(*s_arr[15], 'q', 2, 2);
		test(*s_arr[23], 'q', 4, 4);
		test(*s_arr[13], 'q', 5, C::npos);
		test(*s_arr[7], 'q', 6, C::npos);
		test(*s_arr[5], 'q', 0, 0);
		test(*s_arr[10], 'q', 1, 1);
		test(*s_arr[16], 'q', 5, 5);
		test(*s_arr[17], 'q', 9, 9);
		test(*s_arr[3], 'q', 10, C::npos);
		test(*s_arr[19], 'q', 11, C::npos);
		test(*s_arr[14], 'q', 0, 0);
		test(*s_arr[1], 'q', 1, 1);
		test(*s_arr[8], 'q', 10, 10);
		test(*s_arr[9], 'q', 19, 19);
		test(*s_arr[6], 'q', 20, C::npos);
		test(*s_arr[18], 'q', 21, C::npos);

		test(*s_arr[0], 'q', C::npos);
		test(*s_arr[20], 'q', C::npos);
		test(*s_arr[22], 'q', C::npos);
		test(*s_arr[2], 'q', 0);
		test(*s_arr[4], 'q', 0);
		test(*s_arr[12], 'q', 0);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 24; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
