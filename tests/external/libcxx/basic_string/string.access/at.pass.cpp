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

using C = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<C> s1;
	nvobj::persistent_ptr<C> s2;
	nvobj::persistent_ptr<C> s3;
};

template <class S>
void
test(S &s, typename S::size_type pos)
{
	const S &cs = s;

	if (pos < s.size()) {
		UT_ASSERT(s.at(pos) == s[pos]);
		UT_ASSERT(cs.at(pos) == cs[pos]);
	} else {
		try {
			s.at(pos);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos >= s.size());
		}
		try {
			cs.at(pos);
			UT_ASSERT(false);
		} catch (std::out_of_range &) {
			UT_ASSERT(pos >= s.size());
		}
	}
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
		path, "StringTest: at", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<C>();
			r->s2 = nvobj::make_persistent<C>("123");
			r->s3 = nvobj::make_persistent<C>(
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890");
		});

		test(*r->s1, 0);
		test(*r->s2, 0);
		test(*r->s2, 1);
		test(*r->s2, 2);
		test(*r->s2, 3);
		test(*r->s3, 0);
		test(*r->s3, 64);
		test(*r->s3, r->s3->size());

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->s1);
			nvobj::delete_persistent<C>(r->s2);
			nvobj::delete_persistent<C>(r->s3);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
