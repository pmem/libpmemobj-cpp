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
namespace pmem_exp = nvobj::experimental;
using C = pmem_exp::string;

struct root {
	nvobj::persistent_ptr<C> s1;
	nvobj::persistent_ptr<C> s2;
};

template <class S>
void
test(pmem::obj::pool<root> &pop, S &s)
{
	const S &cs = s;
	for (typename S::size_type i = 0; i < cs.size(); ++i) {
		UT_ASSERT(s[i] == static_cast<char>('0' + i % 10));
		UT_ASSERT(cs[i] == s[i]);
	}
	UT_ASSERT(cs[cs.size()] == '\0');

	nvobj::transaction::run(pop, [&] {
		const nvobj::persistent_ptr<C> s2 = nvobj::make_persistent<C>();
		UT_ASSERT((*s2)[0] == '\0');
		nvobj::delete_persistent<C>(s2);
	});
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
		path, "StringTest: index", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s1 = nvobj::make_persistent<C>("0123456789");
			r->s2 = nvobj::make_persistent<C>(
				"0123456789012345678901234567890123456789"
				"0123456789012345678901234567890123456789"
				"0123456789012345678901234567890123456789"
				"01234567890");
		});

		test(pop, *r->s1);
		test(pop, *r->s2);

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
