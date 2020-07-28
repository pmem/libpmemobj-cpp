// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * v.cpp -- C++ documentation snippets.
 */

#include <fcntl.h>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#define LAYOUT "v_example"

//! [v_property_example]
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

int
main()
{
	pool<root> pop;

	/* open already existing pool */
	try {
		pop = pool<root>::open("example_pool", LAYOUT);
	} catch (const pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "Pool not found" << std::endl;
		return 1;
	}

	v_property_example(pop);

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 2;
	}

	return 0;
}
