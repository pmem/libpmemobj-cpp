// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018, Intel Corporation */

/*
 * v.cpp -- C++ documentation snippets.
 */

//! [v_property_example]
#include <fcntl.h>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

using namespace pmem::obj;
using namespace pmem::obj::experimental;

void
v_property_example()
{
	struct foo {
		foo() : counter(10)
		{
		}
		int counter;
	};

	// pool root structure
	struct root {
		v<foo> f;
	};

	// create a pmemobj pool
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	assert(proot->f.get().counter == 10);

	proot->f.get().counter++;

	assert(proot->f.get().counter == 11);
}
//! [v_property_example]
