//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// <map>

// class map

// size_type max_size() const;

#include <type_traits>

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container = container_t<char, int>;

struct root {
	nvobj::persistent_ptr<container> s;
};

template <class Alloc>
inline typename std::allocator_traits<Alloc>::size_type
alloc_max_size(Alloc const &a)
{
	typedef std::allocator_traits<Alloc> AT;
	return AT::max_size(a);
}

int
run(pmem::obj::pool<root> &pop)
{
	auto robj = pop.root();
#ifdef XXX // XXX: Implement limited_allocator
	typedef std::pair<const int, int> KV;
	{
		typedef limited_allocator<KV, 10> A;
		typedef std::map<int, int, std::less<int>, A> C;
		C c;
		UT_ASSERT(c.max_size() <= 10);
		LIBCPP_ASSERT(c.max_size() == 10);
	}
	{
		typedef limited_allocator<KV, (size_t)-1> A;
		typedef std::map<int, int, std::less<int>, A> C;
		const C::size_type max_dist = static_cast<C::size_type>(
			std::numeric_limits<C::difference_type>::max());
		C c;
		UT_ASSERT(c.max_size() <= max_dist);
		LIBCPP_ASSERT(c.max_size() == max_dist);
	}
#endif
	{
		typedef container C;
		const C::size_type max_dist = static_cast<C::size_type>(
			std::numeric_limits<C::difference_type>::max());
		pmem::obj::transaction::run(
			pop, [&] { robj->s = nvobj::make_persistent<C>(); });
		auto &c = *robj->s;
		UT_ASSERT(c.max_size() <= max_dist);
		pmem::obj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(robj->s); });
	}

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
		pop = pmem::obj::pool<root>::create(path, "max_size.pass",
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
