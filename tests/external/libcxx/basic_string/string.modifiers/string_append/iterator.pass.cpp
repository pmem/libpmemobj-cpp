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

#include "../throwing_iterator.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/string.hpp>

namespace nvobj = pmem::obj;
namespace pmem_exp = pmem::obj::experimental;
using S = pmem_exp::string;

struct root {
	nvobj::persistent_ptr<S> s, s_short, s_long, s_extra_long, a_copy;
	nvobj::persistent_ptr<S> s_arr[19];
};

template <class S, class It>
void
test(nvobj::pool<struct root> &pop, const S &s1, It first, It last,
     const S &expected)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s = nvobj::make_persistent<S>(s1); });

	auto &s = *r->s;

	s.append(first, last);
	UT_ASSERT(s == expected);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s); });
}

template <class S, class It>
void
test_exceptions(nvobj::pool<struct root> &pop, const S &s, It first, It last)
{
	auto r = pop.root();

	nvobj::transaction::run(
		pop, [&] { r->a_copy = nvobj::make_persistent<S>(s); });

	auto &a_copy = *r->a_copy;

	try {
		a_copy.append(first, last);
		UT_ASSERT(false);
	} catch (...) {
	}

	UT_ASSERT(s == a_copy);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<S>(r->a_copy); });
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
		try {
			nvobj::transaction::run(pop, [&] {
				r->s_arr[0] = nvobj::make_persistent<S>();
				r->s_arr[1] = nvobj::make_persistent<S>("A");
				r->s_arr[2] =
					nvobj::make_persistent<S>("ABCDEFGHIJ");
				r->s_arr[3] = nvobj::make_persistent<S>(
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
				r->s_arr[4] =
					nvobj::make_persistent<S>("12345");
				r->s_arr[5] =
					nvobj::make_persistent<S>("12345A");
				r->s_arr[6] = nvobj::make_persistent<S>(
					"12345ABCDEFGHIJ");
				r->s_arr[7] = nvobj::make_persistent<S>(
					"12345ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
				r->s_arr[8] =
					nvobj::make_persistent<S>("1234567890");
				r->s_arr[9] = nvobj::make_persistent<S>(
					"1234567890A");
				r->s_arr[10] = nvobj::make_persistent<S>(
					"1234567890ABCDEFGHIJ");
				r->s_arr[11] = nvobj::make_persistent<S>(
					"1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
				r->s_arr[12] = nvobj::make_persistent<S>(
					"12345678901234567890");
				r->s_arr[13] = nvobj::make_persistent<S>(
					"12345678901234567890"
					"A");
				r->s_arr[14] = nvobj::make_persistent<S>(
					"12345678901234567890"
					"ABCDEFGHIJ");
				r->s_arr[15] = nvobj::make_persistent<S>(
					"12345678901234567890"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
				r->s_arr[16] = nvobj::make_persistent<S>(
					"12345678901234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
					"A");
				r->s_arr[17] = nvobj::make_persistent<S>(
					"12345678901234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJ");
				r->s_arr[18] = nvobj::make_persistent<S>(
					"12345678901234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
			});

			auto &s_arr = r->s_arr;

			const char *s =
				"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

			test(pop, *s_arr[0], s, s, *s_arr[0]);
			test(pop, *s_arr[0], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[0], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[0], s, s + 52, *s_arr[3]);

			test(pop, *s_arr[4], s, s, *s_arr[4]);
			test(pop, *s_arr[4], s, s + 1, *s_arr[5]);
			test(pop, *s_arr[4], s, s + 10, *s_arr[6]);
			test(pop, *s_arr[4], s, s + 52, *s_arr[7]);

			test(pop, *s_arr[8], s, s, *s_arr[8]);
			test(pop, *s_arr[8], s, s + 1, *s_arr[9]);
			test(pop, *s_arr[8], s, s + 10, *s_arr[10]);
			test(pop, *s_arr[8], s, s + 52, *s_arr[11]);

			test(pop, *s_arr[12], s, s, *s_arr[12]);
			test(pop, *s_arr[12], s, s + 1, *s_arr[13]);
			test(pop, *s_arr[12], s, s + 10, *s_arr[14]);
			test(pop, *s_arr[12], s, s + 52, *s_arr[15]);

			test(pop, *s_arr[15], s, s, *s_arr[15]);
			test(pop, *s_arr[15], s, s + 1, *s_arr[16]);
			test(pop, *s_arr[15], s, s + 10, *s_arr[17]);
			test(pop, *s_arr[15], s, s + 52, *s_arr[18]);

			using It = test_support::input_it<const char *>;

			test(pop, *s_arr[0], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[0], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[0], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[0], It(s), It(s + 52), *s_arr[3]);

			test(pop, *s_arr[4], It(s), It(s), *s_arr[4]);
			test(pop, *s_arr[4], It(s), It(s + 1), *s_arr[5]);
			test(pop, *s_arr[4], It(s), It(s + 10), *s_arr[6]);
			test(pop, *s_arr[4], It(s), It(s + 52), *s_arr[7]);

			test(pop, *s_arr[8], It(s), It(s), *s_arr[8]);
			test(pop, *s_arr[8], It(s), It(s + 1), *s_arr[9]);
			test(pop, *s_arr[8], It(s), It(s + 10), *s_arr[10]);
			test(pop, *s_arr[8], It(s), It(s + 52), *s_arr[11]);

			test(pop, *s_arr[12], It(s), It(s), *s_arr[12]);
			test(pop, *s_arr[12], It(s), It(s + 1), *s_arr[13]);
			test(pop, *s_arr[12], It(s), It(s + 10), *s_arr[14]);
			test(pop, *s_arr[12], It(s), It(s + 52), *s_arr[15]);

			test(pop, *s_arr[15], It(s), It(s), *s_arr[15]);
			test(pop, *s_arr[15], It(s), It(s + 1), *s_arr[16]);
			test(pop, *s_arr[15], It(s), It(s + 10), *s_arr[17]);
			test(pop, *s_arr[15], It(s), It(s + 52), *s_arr[18]);

			// test iterator operations that throw
			using TIter = throwing_it<char>;
			using IIter = test_support::forward_it<TIter>;

			test_exceptions(
				pop, *s_arr[0],
				IIter(TIter(s, s + 10, 4, TIter::TAIncrement)),
				IIter());
			test_exceptions(pop, *s_arr[0],
					IIter(TIter(s, s + 10, 5,
						    TIter::TADereference)),
					IIter());
			test_exceptions(
				pop, *s_arr[0],
				IIter(TIter(s, s + 10, 6, TIter::TAComparison)),
				IIter());

			test_exceptions(pop, *s_arr[0],
					TIter(s, s + 10, 4, TIter::TAIncrement),
					TIter());
			test_exceptions(
				pop, *s_arr[0],
				TIter(s, s + 10, 5, TIter::TADereference),
				TIter());
			test_exceptions(
				pop, *s_arr[0],
				TIter(s, s + 10, 6, TIter::TAComparison),
				TIter());

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 19; ++i) {
					nvobj::delete_persistent<S>(s_arr[i]);
				}
			});

			// test appending to self
			nvobj::transaction::run(pop, [&] {
				r->s_short = nvobj::make_persistent<S>("123/");
				r->s_long = nvobj::make_persistent<S>(
					"Lorem ipsum dolor sit amet, consectetur/");
				r->s_extra_long = nvobj::make_persistent<S>(
					"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/");
			});

			auto &s_short = *r->s_short;
			auto &s_long = *r->s_long;
			auto &s_extra_long = *r->s_extra_long;

			s_short.append(s_short.begin(), s_short.end());
			UT_ASSERT(s_short == "123/123/");
			s_short.append(s_short.begin(), s_short.end());
			UT_ASSERT(s_short == "123/123/123/123/");
			s_short.append(s_short.begin(), s_short.end());
			UT_ASSERT(s_short ==
				  "123/123/123/123/123/123/123/123/");

			s_long.append(s_long.begin(), s_long.end());
			UT_ASSERT(
				s_long ==
				"Lorem ipsum dolor sit amet, consectetur/Lorem ipsum dolor sit amet, consectetur/");

			s_extra_long.append(s_extra_long.begin(),
					    s_extra_long.end());
			UT_ASSERT(
				s_extra_long ==
				"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod/");

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<S>(r->s_short);
				nvobj::delete_persistent<S>(r->s_long);
				nvobj::delete_persistent<S>(r->s_extra_long);
			});

			// test appending a different type
			{
				const int8_t p[] = "ABCD";

				nvobj::transaction::run(pop, [&] {
					r->s = nvobj::make_persistent<S>();
				});

				auto &s = *r->s;

				s.append(p, p + 4);
				UT_ASSERT(s == "ABCD");

				nvobj::transaction::run(pop, [&] {
					nvobj::delete_persistent<S>(r->s);
				});
			}

			// test with a move iterator that returns char&&
			{
				using It =
					test_support::forward_it<const char *>;
				typedef std::move_iterator<It> MoveIt;
				const char p[] = "ABCD";

				nvobj::transaction::run(pop, [&] {
					r->s = nvobj::make_persistent<S>();
				});

				auto &s = *r->s;

				s.append(MoveIt(It(std::begin(p))),
					 MoveIt(It(std::end(p) - 1)));
				UT_ASSERT(s == "ABCD");

				nvobj::transaction::run(pop, [&] {
					nvobj::delete_persistent<S>(r->s);
				});
			}

			// test with a move iterator that returns char&&
			{
				typedef const char *It;
				typedef std::move_iterator<It> MoveIt;
				const char p[] = "ABCD";

				nvobj::transaction::run(pop, [&] {
					r->s = nvobj::make_persistent<S>();
				});

				auto &s = *r->s;

				s.append(MoveIt(It(std::begin(p))),
					 MoveIt(It(std::end(p) - 1)));
				UT_ASSERT(s == "ABCD");

				nvobj::transaction::run(pop, [&] {
					nvobj::delete_persistent<S>(r->s);
				});
			}
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();

	return 0;
}
