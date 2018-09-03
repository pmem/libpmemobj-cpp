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
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem_exp = pmem::obj::experimental;
namespace nvobj = pmem::obj;

using string_type = pmem_exp::string;

struct root {
	nvobj::persistent_ptr<string_type> s1;
};

template <class S>
void
test(S &str, unsigned pos, pmem::obj::pool<root> &pop)
{
	typedef typename S::traits_type T;

	auto r = pop.root();

	if (pos <= str.size()) {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<string_type>(str, pos);
		});
		auto &s2 = *r->s1;

		typename S::size_type rlen = str.size() - pos;
		UT_ASSERT(s2.size() == rlen);
		UT_ASSERT(T::compare(s2.cdata(), str.cdata() + pos, rlen) == 0);
		UT_ASSERT(s2.capacity() >= s2.size());

		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(s2.c_str() == s2.data());
			UT_ASSERT(s2.c_str() == s2.cdata());
			UT_ASSERT(s2.c_str() ==
				  static_cast<const string_type &>(s2).data());
		});

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<string_type>(r->s1);
		});
	} else {
		try {
			nvobj::transaction::run(pop, [&] {
				r->s1 = nvobj::make_persistent<string_type>(
					str, pos);
			});
			UT_ASSERT(0);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > str.size());
		}
	}
}

template <class S>
void
test(S &str, unsigned pos, unsigned n, pmem::obj::pool<root> &pop)
{
	typedef typename S::traits_type T;

	auto r = pop.root();

	if (pos <= str.size()) {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<string_type>(str, pos,
								    n);
		});

		auto &s2 = *r->s1;

		typename S::size_type rlen =
			std::min<typename S::size_type>(str.size() - pos, n);
		UT_ASSERT(s2.size() == rlen);
		UT_ASSERT(T::compare(s2.cdata(), str.cdata() + pos, rlen) == 0);
		UT_ASSERT(s2.capacity() >= s2.size());

		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(s2.c_str() == s2.data());
			UT_ASSERT(s2.c_str() == s2.cdata());
			UT_ASSERT(s2.c_str() ==
				  static_cast<const string_type &>(s2).data());
		});

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<string_type>(r->s1);
		});
	} else {
		try {
			nvobj::transaction::run(pop, [&] {
				r->s1 = nvobj::make_persistent<string_type>(
					str, pos, n);
			});
			UT_ASSERT(0);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos > str.size());
		}
	}
}

void
run(pmem::obj::pool<root> &pop)
{
	try {
		nvobj::persistent_ptr<string_type> s_default, s1, s2, s3;

		nvobj::transaction::run(pop, [&] {
			s_default = nvobj::make_persistent<string_type>();
		});

		nvobj::transaction::run(pop, [&] {
			s1 = nvobj::make_persistent<string_type>("1");
		});

		nvobj::transaction::run(pop, [&] {
			s2 = nvobj::make_persistent<string_type>(
				"1234567890123456789012345678901234567890"
				"123456789012345678901234567890");
		});

		nvobj::transaction::run(pop, [&] {
			s3 = nvobj::make_persistent<string_type>(
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890");
		});

		test(*s_default, 0, pop);
		test(*s_default, 1, pop);

		test(*s1, 0, pop);
		test(*s1, 1, pop);
		test(*s1, 2, pop);

		test(*s2, 0, pop);
		test(*s2, 5, pop);
		test(*s2, 50, pop);
		test(*s2, 500, pop);

		test(*s3, 0, pop);
		test(*s3, 5, pop);
		test(*s3, 50, pop);
		test(*s3, 500, pop);

		test(*s_default, 0, 0, pop);
		test(*s_default, 0, 1, pop);
		test(*s_default, 1, 0, pop);
		test(*s_default, 1, 1, pop);
		test(*s_default, 1, 2, pop);

		test(*s1, 0, 0, pop);
		test(*s1, 0, 1, pop);
		test(*s1, 1, 1, pop);

		test(*s2, 0, 5, pop);
		test(*s2, 50, 0, pop);
		test(*s2, 50, 1, pop);
		test(*s2, 50, 10, pop);
		test(*s2, 50, 100, pop);

		test(*s3, 0, 5, pop);
		test(*s3, 50, 0, pop);
		test(*s3, 50, 1, pop);
		test(*s3, 50, 10, pop);
		test(*s3, 50, 100, pop);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<string_type>(s1);
			nvobj::delete_persistent<string_type>(s2);
			nvobj::delete_persistent<string_type>(s3);
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
		pop = pmem::obj::pool<root>::create(path, "string_test",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	pop.close();

	return 0;
}
