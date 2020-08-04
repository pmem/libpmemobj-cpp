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

// <map>

// template <class Key, class T, class Compare = less<Key>,
//           class Allocator = allocator<pair<const Key, T>>>
// class map
// {
// public:
//     // types:
//     typedef Key                                      key_type;
//     typedef T                                        mapped_type;
//     typedef pair<const key_type, mapped_type>        value_type;
//     typedef Compare                                  key_compare;
//     typedef Allocator                                allocator_type;
//     typedef typename allocator_type::reference       reference;
//     typedef typename allocator_type::const_reference const_reference;
//     typedef typename allocator_type::pointer         pointer;
//     typedef typename allocator_type::const_pointer   const_pointer;
//     typedef typename allocator_type::size_type       size_type;
//     typedef typename allocator_type::difference_type difference_type;
//     ...
// };

#include "map_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container = container_t<int, double>;

struct root {
	nvobj::persistent_ptr<container> s;
};

int
run(nvobj::pool<root> &pop)
{
	{
		typedef container C;
		static_assert((std::is_same<C::key_type, int>::value), "");
		static_assert((std::is_same<C::mapped_type, double>::value),
			      "");
		static_assert(
			(std::is_same<
				C::value_type,
				pmem::detail::pair<const int, double>>::value),
			"");
		static_assert(
			(std::is_same<C::key_compare, std::less<int>>::value),
			"");
		static_assert(
			(std::is_same<C::allocator_type,
				      pmem::obj::allocator<pmem::detail::pair<
					      const int, double>>>::value),
			"");
		static_assert(
			(std::is_same<C::reference,
				      pmem::detail::pair<const int, double>
					      &>::value),
			"");
		static_assert(
			(std::is_same<C::const_reference,
				      const pmem::detail::pair<
					      const int, double> &>::value),
			"");
		static_assert(
			(std::is_same<C::pointer,
				      nvobj::persistent_ptr<pmem::detail::pair<
					      const int, double>>>::value),
			"");
		static_assert(
			(std::is_same<
				C::const_pointer,
				nvobj::persistent_ptr<const pmem::detail::pair<
					const int, double>>>::value),
			"");
		static_assert((std::is_same<C::size_type, std::size_t>::value),
			      "");
		static_assert((std::is_same<C::difference_type,
					    std::ptrdiff_t>::value),
			      "");
	}
#ifdef XXX // XXX: Implement min_allocator
	{
		typedef std::map<int, double, std::less<int>,
				 min_allocator<std::pair<const int, double>>>
			C;
		static_assert((std::is_same<C::key_type, int>::value), "");
		static_assert((std::is_same<C::mapped_type, double>::value),
			      "");
		static_assert(
			(std::is_same<C::value_type,
				      std::pair<const int, double>>::value),
			"");
		static_assert(
			(std::is_same<C::key_compare, std::less<int>>::value),
			"");
		static_assert(
			(std::is_same<C::allocator_type,
				      min_allocator<std::pair<const int,
							      double>>>::value),
			"");
		static_assert(
			(std::is_same<C::reference,
				      std::pair<const int, double> &>::value),
			"");
		static_assert(
			(std::is_same<
				C::const_reference,
				const std::pair<const int, double> &>::value),
			"");
		static_assert(
			(std::is_same<C::pointer,
				      min_pointer<std::pair<const int,
							    double>>>::value),
			"");
		static_assert(
			(std::is_same<C::const_pointer,
				      min_pointer<const std::pair<
					      const int, double>>>::value),
			"");
		//  min_allocator doesn't have a size_type, so one gets
		//  synthesized
		static_assert(
			(std::is_same<C::size_type,
				      std::make_unsigned<C::difference_type>::
					      type>::value),
			"");
		static_assert((std::is_same<C::difference_type,
					    std::ptrdiff_t>::value),
			      "");
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
		pop = pmem::obj::pool<root>::create(path, "types.pass",
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
