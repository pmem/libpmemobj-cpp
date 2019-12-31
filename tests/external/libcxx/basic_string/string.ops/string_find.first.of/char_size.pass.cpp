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
	nvobj::persistent_ptr<C> s_arr[22];
};

template <class S>
void
test(const S &s, typename S::value_type c, typename S::size_type pos,
     typename S::size_type x)
{
	UT_ASSERT(s.find_first_of(c, pos) == x);
	if (x != S::npos)
		UT_ASSERT(pos <= x && x < s.size());
}

template <class S>
void
test(const S &s, typename S::value_type c, typename S::size_type x)
{
	UT_ASSERT(s.find_first_of(c) == x);
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
			s_arr[20] = nvobj::make_persistent<C>("qkamf");
			s_arr[21] = nvobj::make_persistent<C>("tpsaf");
		});

		test(*s_arr[0], 'e', 0, C::npos);
		test(*s_arr[0], 'e', 1, C::npos);
		test(*s_arr[11], 'e', 0, C::npos);
		test(*s_arr[20], 'e', 1, C::npos);
		test(*s_arr[15], 'e', 2, C::npos);
		test(*s_arr[21], 'e', 4, C::npos);
		test(*s_arr[13], 'e', 5, C::npos);
		test(*s_arr[7], 'e', 6, C::npos);
		test(*s_arr[5], 'e', 0, C::npos);
		test(*s_arr[10], 'e', 1, 4);
		test(*s_arr[16], 'e', 5, 6);
		test(*s_arr[17], 'e', 9, C::npos);
		test(*s_arr[3], 'e', 10, C::npos);
		test(*s_arr[19], 'e', 11, C::npos);
		test(*s_arr[14], 'e', 0, 7);
		test(*s_arr[1], 'e', 1, 4);
		test(*s_arr[8], 'e', 10, C::npos);
		test(*s_arr[9], 'e', 19, C::npos);
		test(*s_arr[6], 'e', 20, C::npos);
		test(*s_arr[18], 'e', 21, C::npos);

		test(*s_arr[0], 'e', C::npos);
		test(*s_arr[2], 'e', 4);
		test(*s_arr[4], 'e', C::npos);
		test(*s_arr[12], 'e', 2);

		nvobj::transaction::run(pop, [&] {
			for (unsigned i = 0; i < 22; ++i) {
				nvobj::delete_persistent<C>(s_arr[i]);
			}
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
