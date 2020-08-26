// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/experimental/radix_tree.hpp>

#include <libpmemobj/iterator.h>

namespace nvobj = pmem::obj;

struct bytes_view {
	bytes_view(const nvobj::string *s) : s(s)
	{
	}

	char operator[](std::size_t p) const
	{
		return (*s)[p];
	}

	size_t
	size() const
	{
		return s->size();
	}

	const nvobj::string *s;
};

using container_t =
	nvobj::experimental::radix_tree<nvobj::string, int, bytes_view>;

struct root {
	nvobj::persistent_ptr<nvobj::string> str;
	nvobj::persistent_ptr<container_t> map;
};

void
test_long_string(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->map = nvobj::make_persistent<container_t>();
		r->str = nvobj::make_persistent<nvobj::string>((1ULL << 32),
							       'a');
	});

	auto ret = r->map->try_emplace(*r->str, 0);
	UT_ASSERT(ret.second);

	ret = r->map->try_emplace(std::move(*r->str), 1);
	UT_ASSERT(!ret.second);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_t>(r->map);
		nvobj::delete_persistent<nvobj::string>(r->str);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<struct root>::create(path, "radix_basic",
						       3000 * PMEMOBJ_MIN_POOL,
						       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_long_string(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
