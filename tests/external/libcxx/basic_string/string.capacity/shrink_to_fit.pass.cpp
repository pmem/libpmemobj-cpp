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
	nvobj::persistent_ptr<S> s, s0;
};

template <class S>
void
test(nvobj::pool<struct root> &pop, S &s)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s0 = nvobj::make_persistent<S>(s); });

	auto &s0 = *r->s0;

	typename S::size_type old_cap = s.capacity();
	s.shrink_to_fit();
	UT_ASSERT(s == s0);
	UT_ASSERT(s.capacity() <= old_cap);
	UT_ASSERT(s.capacity() >= s.size());

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s0); });
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
				r->s = nvobj::make_persistent<S>();
			});

			auto &s = *r->s;

			test(pop, s);

			s.assign(10, 'a');
			s.erase(5);
			test(pop, s);

			s.assign(50, 'a');
			s.erase(5);
			test(pop, s);

			s.assign(100, 'a');
			s.erase(50);
			test(pop, s);

			s.assign(200, 'a');
			s.erase(100);
			test(pop, s);

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
