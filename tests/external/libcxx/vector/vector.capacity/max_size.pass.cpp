//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019-2020, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using C = container_t<char>;

struct root {
	nvobj::persistent_ptr<C> v;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VectorTest: max_size",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		const C::difference_type max_dist =
			static_cast<C::difference_type>(
				std::numeric_limits<C::difference_type>::max());

		nvobj::transaction::run(
			pop, [&] { r->v = nvobj::make_persistent<C>(); });

		UT_ASSERT(r->v->max_size() <= max_dist);

		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v); });

	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
