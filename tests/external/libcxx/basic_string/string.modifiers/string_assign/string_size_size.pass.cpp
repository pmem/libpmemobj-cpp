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
	nvobj::persistent_ptr<S> s;
	nvobj::persistent_ptr<S> s_arr[17];
};

template <class S>
void
test(nvobj::pool<struct root> &pop, const S &s1, const S &str,
     typename S::size_type pos, typename S::size_type n, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	if (pos <= str.size()) {
		s.assign(str, pos, n);
		UT_ASSERT(s == expected);
	} else {
		try {
			s.assign(str, pos, n);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > str.size());
		}
	}

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s); });
}

template <class S>
void
test_npos(nvobj::pool<struct root> &pop, const S &s1, const S &str,
	  typename S::size_type pos, const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	if (pos <= str.size()) {
		s.assign(str, pos);
		UT_ASSERT(s == expected);
	} else {
		try {
			s.assign(str, pos);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > str.size());
		}
	}

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

	auto &s_arr = pop.root()->s_arr;
	{
		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<S>();
				s_arr[1] = nvobj::make_persistent<S>("123");
				s_arr[2] = nvobj::make_persistent<S>("12345");
				s_arr[3] = nvobj::make_persistent<S>("2345");
				s_arr[4] = nvobj::make_persistent<S>("45");
				s_arr[5] = nvobj::make_persistent<S>("");
				s_arr[6] = nvobj::make_persistent<S>(
					"not happening");
				s_arr[7] = nvobj::make_persistent<S>(
					"12345678901234567890");
				s_arr[8] = nvobj::make_persistent<S>("2");
				s_arr[9] = nvobj::make_persistent<S>("345");
				s_arr[10] =
					nvobj::make_persistent<S>("34567890");
				s_arr[11] = nvobj::make_persistent<S>("34");
				s_arr[12] =
					nvobj::make_persistent<S>("1234567890");
				s_arr[13] =
					nvobj::make_persistent<S>("34567890");
				s_arr[14] = nvobj::make_persistent<S>("234");
				s_arr[15] =
					nvobj::make_persistent<S>("6789012345");
				s_arr[16] = nvobj::make_persistent<S>(
					"12345678901234567890123456789012345678901234567890123456789012345678901234567890");
			});

			test(pop, *s_arr[0], *s_arr[0], 0, 0, *s_arr[0]);
			test(pop, *s_arr[0], *s_arr[0], 1, 0, *s_arr[0]);
			test(pop, *s_arr[0], *s_arr[2], 0, 3, *s_arr[1]);
			test(pop, *s_arr[0], *s_arr[2], 1, 4, *s_arr[3]);
			test(pop, *s_arr[0], *s_arr[2], 3, 15, *s_arr[4]);
			test(pop, *s_arr[0], *s_arr[2], 5, 15, *s_arr[5]);
			test(pop, *s_arr[0], *s_arr[2], 6, 15, *s_arr[6]);
			test(pop, *s_arr[0], *s_arr[7], 0, 0, *s_arr[0]);
			test(pop, *s_arr[0], *s_arr[7], 1, 1, *s_arr[8]);
			test(pop, *s_arr[0], *s_arr[7], 2, 3, *s_arr[9]);
			test(pop, *s_arr[0], *s_arr[7], 12, 13, *s_arr[10]);
			test(pop, *s_arr[0], *s_arr[7], 21, 13, *s_arr[6]);
			test(pop, *s_arr[0], *s_arr[16], 10, 20, *s_arr[7]);
			test(pop, *s_arr[0], *s_arr[16], 0, 80, *s_arr[16]);

			test(pop, *s_arr[2], *s_arr[0], 0, 0, *s_arr[0]);
			test(pop, *s_arr[2], *s_arr[2], 2, 2, *s_arr[11]);
			test(pop, *s_arr[2], *s_arr[12], 0, 100, *s_arr[12]);

			test(pop, *s_arr[7], *s_arr[0], 0, 0, *s_arr[0]);
			test(pop, *s_arr[7], *s_arr[2], 1, 3, *s_arr[14]);
			test(pop, *s_arr[7], *s_arr[7], 5, 10, *s_arr[15]);

			test(pop, *s_arr[16], *s_arr[0], 0, 0, *s_arr[0]);
			test(pop, *s_arr[16], *s_arr[2], 1, 3, *s_arr[14]);
			test(pop, *s_arr[16], *s_arr[7], 5, 10, *s_arr[15]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 17; ++i) {
					nvobj::delete_persistent<S>(s_arr[i]);
				}
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		{
			try {
				nvobj::transaction::run(pop, [&] {
					s_arr[0] = nvobj::make_persistent<S>();
					s_arr[1] = nvobj::make_persistent<S>(
						"12345");
					s_arr[2] = nvobj::make_persistent<S>(
						"2345");
					s_arr[3] =
						nvobj::make_persistent<S>("45");
					s_arr[4] =
						nvobj::make_persistent<S>("");
					s_arr[5] = nvobj::make_persistent<S>(
						"not happening");
					s_arr[6] = nvobj::make_persistent<S>(
						"12345678901234567890123456789012345678901234567890123456789012345678901234567890");
				});

				test_npos(pop, *s_arr[0], *s_arr[0], 0,
					  *s_arr[0]);
				test_npos(pop, *s_arr[0], *s_arr[0], 1,
					  *s_arr[0]);

				test_npos(pop, *s_arr[0], *s_arr[1], 0,
					  *s_arr[1]);
				test_npos(pop, *s_arr[0], *s_arr[1], 1,
					  *s_arr[2]);
				test_npos(pop, *s_arr[0], *s_arr[1], 3,
					  *s_arr[3]);
				test_npos(pop, *s_arr[0], *s_arr[1], 5,
					  *s_arr[4]);
				test_npos(pop, *s_arr[0], *s_arr[1], 6,
					  *s_arr[5]);
				test_npos(pop, *s_arr[0], *s_arr[6], 0,
					  *s_arr[6]);
				test_npos(pop, *s_arr[6], *s_arr[1], 1,
					  *s_arr[2]);

				nvobj::transaction::run(pop, [&] {
					for (unsigned i = 0; i < 7; ++i) {
						nvobj::delete_persistent<S>(
							s_arr[i]);
					}
				});
			} catch (std::exception &e) {
				UT_FATALexc(e);
			}
		}
	}

	pop.close();

	return 0;
}
