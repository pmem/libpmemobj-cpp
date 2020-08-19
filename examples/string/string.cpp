// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include <iostream>
#include <libpmemobj++/container/string.hpp>

//! [string_example]
using namespace pmem;
using namespace pmem::obj;

/* example object containing string */
struct example_object {
	example_object(){};
	example_object(const char *s) : string_inside(s){};

	string string_inside;
};

/* pool root structure */
struct root {
	persistent_ptr<string> test_string = nullptr;
	persistent_ptr<example_object> test_object = nullptr;
};

/* example with persistent_ptr<string> */
void
example_with_ptr(pool<root> &pop)
{
	auto r = pop.root();

	if (r->test_string == nullptr) {
		transaction::run(pop, [&] {
			/* note that allocation in transaction is made by
			 * make_persistent */
			r->test_string = make_persistent<string>("example1");
		});
	}

	std::cout << r->test_string->c_str() << std::endl;

	transaction::run(pop, [&] {
		delete_persistent<string>(r->test_string);
		r->test_string = nullptr;
	});
}

/* example with object containing string */
void
example_with_object(pool<root> &pop)
{
	auto r = pop.root();

	if (r->test_object == nullptr) {
		transaction::run(pop, [&] {
			/* WRONG:
			 * r->test_object = make_persistent<example_object>();
			 * r->test_object->string_inside = string("example2");
			 */

			/* GOOD: */
			r->test_object =
				make_persistent<example_object>("example2");
			/* OR:
			 * r->test_object = make_persistent<example_object>();
			 * r->test_object->string_inside.assign("example2");
			 */
		});
	}

	std::cout << r->test_object->string_inside.c_str() << std::endl;

	transaction::run(pop, [&] {
		delete_persistent<example_object>(r->test_object);
		r->test_object = nullptr;
	});
}
//! [string_example]

/* Before running this example, run:
 * pmempool create obj --layout="string_example" example_pool
 */
int
main()
{
	pool<root> pop;

	/* open already existing pool */
	try {
		pop = pool<root>::open("example_pool", "string_example");
	} catch (const pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "Pool not found" << std::endl;
		return 1;
	}

	try {
		example_with_ptr(pop);
		example_with_object(pop);
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
