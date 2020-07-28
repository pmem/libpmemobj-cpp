// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * v.cpp -- C++ documentation snippets.
 */

#include <iostream>
//! [v_property_example]
#include <fcntl.h>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

using namespace pmem::obj;
using namespace pmem::obj::experimental;

struct foo {
	foo() : counter(10){};
	int counter;
};

/* pool root structure */
struct root {
	v<foo> f;
};

void
v_property_example(pool<root> &pop)
{
	auto proot = pop.root();

	assert(proot->f.get().counter == 10);

	proot->f.get().counter++;

	assert(proot->f.get().counter == 11);
}
//! [v_property_example]

/* Before running this example, run:
 * pmempool create obj --layout="v_example" example_pool
 */
int
main()
{
	pool<root> pop;

	/* open already existing pool */
	try {
		pop = pool<root>::open("example_pool", "v_example");
	} catch (const pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "Pool not found" << std::endl;
		return 1;
	}

	try {
		v_property_example(pop);
	} catch (const std::exception &e) {
		std::cerr << "Exception " << e.what() << std::endl;
		return -1;
	}

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 2;
	}

	return 0;
}
