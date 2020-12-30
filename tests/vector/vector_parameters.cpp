// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;

using container = container_t<int>;

struct root {
	nvobj::persistent_ptr<container> pptr;
};

void
check_if_empty(nvobj::persistent_ptr<container> &c)
{
	UT_ASSERT(c->size() == 0);
	UT_ASSERT(c->capacity() == 0);
}

/**
 * Testing container methods with parameters = 0
 */
void
zero_test(nvobj::pool<struct root> &pop)
{
	auto &c = pop.root()->pptr;

	/* ctor test */
	nvobj::transaction::run(
		pop, [&] { c = nvobj::make_persistent<container>(0U); });

	auto list = std::initializer_list<int>({});

	/* assign test */
	// calling on empty container to check when segment=vector
	c->assign(0, 0);
	check_if_empty(c);
	// for size() == 1, to call shrink inside next assign
	c->assign(1, 0);
	c->assign(list.begin(), list.end());
	c->free_data();
	check_if_empty(c);

	/* insert test */
	c->insert(c->cbegin(), 0, 0);
	check_if_empty(c);

	/* resize test */
	c->resize(0);
	check_if_empty(c);

	/* shrink_to_fit test */
	c->shrink_to_fit();
	check_if_empty(c);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<container>(c); });
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: modifiers_exceptions_oom",
		PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	try {
		zero_test(pop);
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
