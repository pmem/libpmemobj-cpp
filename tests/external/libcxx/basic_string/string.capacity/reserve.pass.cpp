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

using S = pmem::obj::string;

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
	s.reserve();
	UT_ASSERT(s == s0);
	UT_ASSERT(s.capacity() <= old_cap);
	UT_ASSERT(s.capacity() >= s.size());

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<S>(r->s0); });
}

template <class S>
void
test(nvobj::pool<struct root> &pop, S &s, typename S::size_type res_arg)
{
	auto r = pop.root();

	nvobj::transaction::run(pop,
				[&] { r->s0 = nvobj::make_persistent<S>(s); });

	typename S::size_type old_cap = s.capacity();
	((void)old_cap); // Prevent unused warning

	auto &s0 = *r->s0;

	if (res_arg <= s.max_size()) {
		s.reserve(res_arg);
		UT_ASSERT(s == s0);
		UT_ASSERT(s.capacity() >= res_arg);
		UT_ASSERT(s.capacity() >= s.size());
	} else {
		try {
			s.reserve(res_arg);
			UT_ASSERT(false);
		} catch (std::length_error &) {
			UT_ASSERT(res_arg > s.max_size());
		}
	}

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
			{
				nvobj::transaction::run(pop, [&] {
					r->s = nvobj::make_persistent<S>();
				});

				auto &s = *r->s;

				test(pop, s);

				s.assign(10, 'a');
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
			}
			{
				nvobj::transaction::run(pop, [&] {
					r->s = nvobj::make_persistent<S>(100U,
									 'a');
				});

				auto &s = *r->s;

				s.erase(50);
				test(pop, s, 5);
				test(pop, s, 10);
				test(pop, s, 50);
				test(pop, s, 100);
				test(pop, s, S::npos);

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
