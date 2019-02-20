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

namespace nvobj = pmem::obj;
namespace pmem_exp = pmem::obj::experimental;
using S = pmem_exp::string;

struct root {
	nvobj::persistent_ptr<S> s, s_short, s_long;
	nvobj::persistent_ptr<S> s_arr[7];
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
	// XXX: enable operator==
	// auto &expected = *r->expected;

	s.assign(first, last);

	// XXX: enable operator==
	// UT_ASSERT(s == expected);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s); });
}

//#ifndef TEST_HAS_NO_EXCEPTIONS
// template <class S, class It>
// void
// test_exceptions(S s, It first, It last)
//{
//    S aCopy = s;
//    try {
//        s.assign(first, last);
//        assert(false);
//    }
//    catch (...) {}
//    LIBCPP_ASSERT(s.__invariants());
//    assert(s == aCopy);
//}
//#endif

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
		const char *s =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
		try {
			nvobj::transaction::run(pop, [&] {
				s_arr[0] = nvobj::make_persistent<S>();
				s_arr[1] = nvobj::make_persistent<S>("A");
				s_arr[2] =
					nvobj::make_persistent<S>("ABCDEFGHIJ");
				s_arr[3] = nvobj::make_persistent<S>(
					"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
				s_arr[4] = nvobj::make_persistent<S>("12345");
				s_arr[5] =
					nvobj::make_persistent<S>("1234567890");
				s_arr[6] = nvobj::make_persistent<S>(
					"12345678901234567890");
			});

			test(pop, *s_arr[0], s, s, *s_arr[0]);
			test(pop, *s_arr[0], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[0], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[0], s, s + 52, *s_arr[3]);

			test(pop, *s_arr[4], s, s, *s_arr[0]);
			test(pop, *s_arr[4], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[4], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[4], s, s + 52, *s_arr[3]);

			test(pop, *s_arr[5], s, s, *s_arr[0]);
			test(pop, *s_arr[5], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[5], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[5], s, s + 52, *s_arr[3]);

			test(pop, *s_arr[6], s, s, *s_arr[0]);
			test(pop, *s_arr[6], s, s + 1, *s_arr[1]);
			test(pop, *s_arr[6], s, s + 10, *s_arr[2]);
			test(pop, *s_arr[6], s, s + 52, *s_arr[3]);

			using It = test_support::input_it<const char *>;

			test(pop, *s_arr[0], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[0], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[0], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[0], It(s), It(s + 52), *s_arr[3]);

			test(pop, *s_arr[4], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[4], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[4], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[4], It(s), It(s + 52), *s_arr[3]);

			test(pop, *s_arr[5], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[5], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[5], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[5], It(s), It(s + 52), *s_arr[3]);

			test(pop, *s_arr[6], It(s), It(s), *s_arr[0]);
			test(pop, *s_arr[6], It(s), It(s + 1), *s_arr[1]);
			test(pop, *s_arr[6], It(s), It(s + 10), *s_arr[2]);
			test(pop, *s_arr[6], It(s), It(s + 52), *s_arr[3]);

			nvobj::transaction::run(pop, [&] {
				for (unsigned i = 0; i < 7; ++i) {
					nvobj::delete_persistent<S>(s_arr[i]);
				}
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{ // test assigning to self
		try {
			nvobj::transaction::run(pop, [&] {
				r->s_short = nvobj::make_persistent<S>("123/");
				r->s_long = nvobj::make_persistent<S>(
					"Lorem ipsum dolor sit amet, consectetur/");
			});

			auto &s_short = *r->s_short;
			auto &s_long = *r->s_long;

			s_short.assign(s_short.begin(), s_short.end());
			// XXX: enable operator==
			// UT_ASSERT(s_short == "123/");
			s_short.assign(s_short.begin() + 2, s_short.end());
			// XXX: enable operator==
			// UT_ASSERT(s_short == "3/");

			s_long.assign(s_long.begin(), s_long.end());
			// XXX: enable operator==
			// UT_ASSERT(s_long == "Lorem ipsum dolor sit amet,
			// consectetur/");

			s_long.assign(s_long.begin() + 30, s_long.end());
			// XXX: enable operator==
			// UT_ASSERT(s_long == "nsectetur/");

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<S>(r->s_short);
				nvobj::delete_persistent<S>(r->s_long);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{ // test assigning a different type
		const int8_t p[] = "ABCD";
		try {
			nvobj::transaction::run(pop, [&] {
				r->s = nvobj::make_persistent<S>();
			});

			auto &s = *r->s;

			s.assign(p, p + 4);
			// XXX: enable operator==
			// UT_ASSERT(s == "ABCD");

			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<S>(r->s);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}

	pop.close();

	return 0;
}

// int main()
//{
//#ifndef TEST_HAS_NO_EXCEPTIONS
//    { // test iterator operations that throw
//    typedef std::string S;
//    typedef ThrowingIterator<char> TIter;
//    typedef input_iterator<TIter> IIter;
//    const char* s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
//    test_exceptions(S(), IIter(TIter(s, s+10, 4, TIter::TAIncrement)),
//    IIter()); test_exceptions(S(), IIter(TIter(s, s+10, 5,
//    TIter::TADereference)), IIter()); test_exceptions(S(), IIter(TIter(s,
//    s+10, 6, TIter::TAComparison)), IIter());
//
//    test_exceptions(S(), TIter(s, s+10, 4, TIter::TAIncrement), TIter());
//    test_exceptions(S(), TIter(s, s+10, 5, TIter::TADereference), TIter());
//    test_exceptions(S(), TIter(s, s+10, 6, TIter::TAComparison), TIter());
//    }
//#endif
