// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <iostream>
#include <libpmemobj++/container/string.hpp>

//! [string_example]
#define LAYOUT "string_example"

using namespace pmem;
using namespace pmem::obj;

/* pool root structure */
struct root {
	/* note that use of persistent_ptr<string> type
	 * is required instead of string
	 */
	persistent_ptr<string> test_string = nullptr;
};

int
main()
{
	pool<root> pop;

	/* create or open already existing pool */
	try {
		pop = pool<root>::open("example_pool", LAYOUT);
	} catch (const pool_error &e) {
		pop = pool<root>::create("example_pool", LAYOUT);
	}
	auto r = pop.root();

	if (r->test_string == nullptr)
	{
		transaction::run(pop, [&] {
			/* note that allocation in transaction is made by make_persistent
			 * use of simple string() constructor isn't proper for persistent memory
			 */
			r->test_string = make_persistent<string>("example");
		});
	}

	std::cout << r->test_string->c_str() << std::endl;

	transaction::run(pop, [&] {
		delete_persistent<string>(r->test_string);
		r->test_string = nullptr;
	});

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}
//! [string_example]
