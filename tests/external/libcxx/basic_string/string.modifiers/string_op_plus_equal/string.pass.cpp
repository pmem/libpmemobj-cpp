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
	nvobj::persistent_ptr<S> s, s_arr[17];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, const S &str,
     const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	s += str;
	UT_ASSERT(s == expected);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s); });
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

	auto r = pop.root();
	{
		auto &s_arr = r->s_arr;

		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<S>();
				s_arr[1] = nvobj::make_persistent<S>("12345");
				s_arr[2] =
					nvobj::make_persistent<S>("1234567890");
				s_arr[3] = nvobj::make_persistent<S>(
					"12345678901234567890");
				s_arr[4] =
					nvobj::make_persistent<S>("1234512345");
				s_arr[5] = nvobj::make_persistent<S>(
					"123451234567890");
				s_arr[6] = nvobj::make_persistent<S>(
					"1234512345678901234567890");
				s_arr[7] = nvobj::make_persistent<S>(
					"123456789012345678901234567890");
				s_arr[8] = nvobj::make_persistent<S>(
					"1234567890123456789012345");
				s_arr[9] = nvobj::make_persistent<S>(
					"1234567890123456789012345678901234567890");
				s_arr[10] = nvobj::make_persistent<S>(
					"123456789012345");

				s_arr[11] = nvobj::make_persistent<S>(
					"123456789012345678901234567890123456789012345678901234567890");
				s_arr[12] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890123456789012345");
				s_arr[13] = nvobj::make_persistent<S>(
					"1234567890123456789012345678901234567890123456789012345678901234567890");
				s_arr[14] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890123456789012345678901234567890");
				s_arr[15] = nvobj::make_persistent<S>(
					"123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");
				s_arr[16] = nvobj::make_persistent<S>(
					"123456789012345678901234567890123456789012345678901234567890123456789012345");
			});

			test(pop, *s_arr[0], *s_arr[0], *s_arr[0]);
			test(pop, *s_arr[0], *s_arr[1], *s_arr[1]);
			test(pop, *s_arr[0], *s_arr[2], *s_arr[2]);
			test(pop, *s_arr[0], *s_arr[3], *s_arr[3]);

			test(pop, *s_arr[1], *s_arr[0], *s_arr[1]);
			test(pop, *s_arr[1], *s_arr[1], *s_arr[4]);
			test(pop, *s_arr[1], *s_arr[2], *s_arr[5]);
			test(pop, *s_arr[1], *s_arr[3], *s_arr[6]);

			test(pop, *s_arr[2], *s_arr[0], *s_arr[2]);
			test(pop, *s_arr[2], *s_arr[1], *s_arr[10]);
			test(pop, *s_arr[2], *s_arr[2], *s_arr[3]);
			test(pop, *s_arr[2], *s_arr[3], *s_arr[7]);

			test(pop, *s_arr[11], *s_arr[0], *s_arr[11]);
			test(pop, *s_arr[11], *s_arr[1], *s_arr[12]);
			test(pop, *s_arr[11], *s_arr[2], *s_arr[13]);
			test(pop, *s_arr[11], *s_arr[3], *s_arr[14]);

			test(pop, *s_arr[13], *s_arr[0], *s_arr[13]);
			test(pop, *s_arr[13], *s_arr[1], *s_arr[16]);
			test(pop, *s_arr[13], *s_arr[2], *s_arr[14]);
			test(pop, *s_arr[13], *s_arr[3], *s_arr[15]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 17; ++i) {
					nvobj::delete_persistent<S>(s_arr[i]);
				}
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();

	return 0;
}
