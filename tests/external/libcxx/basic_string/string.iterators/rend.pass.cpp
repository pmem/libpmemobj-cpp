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

using C = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<C> s1, s2;
};

template <class S>
void
test(S &s)
{
	const S &cs = s;
	typename S::reverse_iterator e = s.rend();
	typename S::const_reverse_iterator ce = cs.rend();
	if (s.empty()) {
		UT_ASSERT(e == s.rbegin());
		UT_ASSERT(ce == cs.rbegin());
	}
	UT_ASSERT(static_cast<std::size_t>(e - s.rbegin()) == s.size());
	UT_ASSERT(static_cast<std::size_t>(ce - cs.rbegin()) == cs.size());
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
		path, "StringTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<C>();
			r->s2 = nvobj::make_persistent<C>("123");
		});

		test(*r->s1);
		test(*r->s2);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->s1);
			nvobj::delete_persistent<C>(r->s2);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();

	return 0;
}
