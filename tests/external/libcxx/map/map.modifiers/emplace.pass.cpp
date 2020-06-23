//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Copyright 2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

// UNSUPPORTED: c++98, c++03

// <map>

// class map

// template <class... Args>
//   pair<iterator, bool> emplace(Args&&... args);

#include "../default_only.hpp"
#include "../emplaceable.hpp"
#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <functional>
#include <tuple>

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container = container_t<int, DefaultOnly>;
using container2 = container_t<int, Emplaceable>;
using container3 = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<container> s;
	nvobj::persistent_ptr<container2> s2;
	nvobj::persistent_ptr<container3> s3;
};

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
	{
		typedef container M;
		typedef std::pair<M::iterator, bool> R;
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<M>(); });
		auto &m = *robj->s;
		(void)m;
		UT_ASSERT(DefaultOnly::count == 0);
		R r = m.emplace();
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(m.begin()->MAP_KEY == 0);
		UT_ASSERT(m.begin()->MAP_VALUE == DefaultOnly());
		UT_ASSERT(DefaultOnly::count == 1);
		r = m.emplace(std::piecewise_construct,
			      std::forward_as_tuple(1),
			      std::forward_as_tuple());
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == std::next(m.begin()));
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(std::next(m.begin())->MAP_KEY == 1);
		UT_ASSERT(std::next(m.begin())->MAP_VALUE == DefaultOnly());
		UT_ASSERT(DefaultOnly::count == 2);
		r = m.emplace(std::piecewise_construct,
			      std::forward_as_tuple(1),
			      std::forward_as_tuple());
		UT_ASSERT(!r.second);
		UT_ASSERT(r.first == std::next(m.begin()));
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(std::next(m.begin())->MAP_KEY == 1);
		UT_ASSERT(std::next(m.begin())->MAP_VALUE == DefaultOnly());
		UT_ASSERT(DefaultOnly::count == 2);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s); });
	}
	UT_ASSERT(DefaultOnly::count == 0);
	{
		typedef container2 M;
		typedef std::pair<M::iterator, bool> R;
		pmem::obj::transaction::run(
			pop, [&] { robj->s2 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s2;
		R r = m.emplace(std::piecewise_construct,
				std::forward_as_tuple(2),
				std::forward_as_tuple());
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(m.begin()->MAP_KEY == 2);
		UT_ASSERT(m.begin()->MAP_VALUE == Emplaceable());
		r = m.emplace(std::piecewise_construct,
			      std::forward_as_tuple(1),
			      std::forward_as_tuple(2, 3.5));
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(m.begin()->MAP_KEY == 1);
		UT_ASSERT(m.begin()->MAP_VALUE == Emplaceable(2, 3.5));
		r = m.emplace(std::piecewise_construct,
			      std::forward_as_tuple(1),
			      std::forward_as_tuple(2, 3.5));
		UT_ASSERT(!r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(m.begin()->MAP_KEY == 1);
		UT_ASSERT(m.begin()->MAP_VALUE == Emplaceable(2, 3.5));
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s2); });
	}
	{
		typedef container3 M;
		typedef std::pair<M::iterator, bool> R;
		pmem::obj::transaction::run(
			pop, [&] { robj->s3 = nvobj::make_persistent<M>(); });
		auto &m = *robj->s3;
		R r = m.emplace(M::value_type(2, 3.5));
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(m.begin()->MAP_KEY == 2);
		UT_ASSERT(m.begin()->MAP_VALUE == 3.5);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<M>(robj->s3); });
	}
#ifdef XXX // XXX: Implement min_allocator class and generic std::less
	{
		typedef std::map<
			int, DefaultOnly, std::less<int>,
			min_allocator<std::pair<const int, DefaultOnly>>>
			M;
		typedef std::pair<M::iterator, bool> R;
		M m;
		UT_ASSERT(DefaultOnly::count == 0);
		R r = m.emplace();
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(m.begin()->MAP_KEY == 0);
		UT_ASSERT(m.begin()->MAP_VALUE == DefaultOnly());
		UT_ASSERT(DefaultOnly::count == 1);
		r = m.emplace(std::piecewise_construct,
			      std::forward_as_tuple(1),
			      std::forward_as_tuple());
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == std::next(m.begin()));
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(std::next(m.begin())->MAP_KEY == 1);
		UT_ASSERT(std::next(m.begin())->MAP_VALUE == DefaultOnly());
		UT_ASSERT(DefaultOnly::count == 2);
		r = m.emplace(std::piecewise_construct,
			      std::forward_as_tuple(1),
			      std::forward_as_tuple());
		UT_ASSERT(!r.second);
		UT_ASSERT(r.first == std::next(m.begin()));
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(std::next(m.begin())->MAP_KEY == 1);
		UT_ASSERT(std::next(m.begin())->MAP_VALUE == DefaultOnly());
		UT_ASSERT(DefaultOnly::count == 2);
	}
	UT_ASSERT(DefaultOnly::count == 0);
	{
		typedef std::map<
			int, Emplaceable, std::less<int>,
			min_allocator<std::pair<const int, Emplaceable>>>
			M;
		typedef std::pair<M::iterator, bool> R;
		M m;
		R r = m.emplace(std::piecewise_construct,
				std::forward_as_tuple(2),
				std::forward_as_tuple());
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(m.begin()->MAP_KEY == 2);
		UT_ASSERT(m.begin()->MAP_VALUE == Emplaceable());
		r = m.emplace(std::piecewise_construct,
			      std::forward_as_tuple(1),
			      std::forward_as_tuple(2, 3.5));
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(m.begin()->MAP_KEY == 1);
		UT_ASSERT(m.begin()->MAP_VALUE == Emplaceable(2, 3.5));
		r = m.emplace(std::piecewise_construct,
			      std::forward_as_tuple(1),
			      std::forward_as_tuple(2, 3.5));
		UT_ASSERT(!r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 2);
		UT_ASSERT(m.begin()->MAP_KEY == 1);
		UT_ASSERT(m.begin()->MAP_VALUE == Emplaceable(2, 3.5));
	}
	{
		typedef std::map<int, double, std::less<int>,
				 min_allocator<std::pair<const int, double>>>
			M;
		typedef std::pair<M::iterator, bool> R;
		M m;
		R r = m.emplace(M::value_type(2, 3.5));
		UT_ASSERT(r.second);
		UT_ASSERT(r.first == m.begin());
		UT_ASSERT(m.size() == 1);
		UT_ASSERT(m.begin()->MAP_KEY == 2);
		UT_ASSERT(m.begin()->MAP_VALUE == 3.5);
	}
#endif

	return 0;
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;
	try {
		pop = pmem::obj::pool<root>::create(path, "emplace.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}
	try {
		run(pop);
		pop.close();
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
