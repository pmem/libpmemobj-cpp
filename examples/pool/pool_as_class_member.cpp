// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/*
 * pool_as_class_member.cpp -- Example shows how to manage pool using RAII
 * idiom.
 */

#include <iostream>
//! [pool_class_member_example]
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

using namespace pmem::obj;

class Foo {
private:
	/* pool root structure */
	struct persistent_data {
		p<int> some_variable;
	};

	/* pool object */
	pmem::obj::pool<persistent_data> pop;
	/* Pointer to data. */

	static constexpr const char *layout = "pool_layout";

public:
	Foo(const char *poolfile_path)
	{
		/* Create a pmemobj pool. */
		try {
			pop = pool<persistent_data>::open(poolfile_path,
							  layout);
		} catch (pmem::pool_error &e) {
			std::cerr << "Cannot open pool: " << e.what()
				  << std::endl
				  << "Trying to create new one " << std::endl;
			pop = pool<persistent_data>::create("poolfile", layout,
							    PMEMOBJ_MIN_POOL);
		}
		/* Assign persistent poiner to root object. */
	}

	~Foo()
	{
		/* Close a pmemobj pool */
		pop.close();
	}

	void
	set(int variable)
	{
		auto root_obj = pop.root();
		root_obj->some_variable = variable;
		pop.persist(root_obj->some_variable);
	}

	void
	increment()
	{
		auto root_obj = pop.root();
		root_obj->some_variable = root_obj->some_variable + 1;
		pop.persist(root_obj->some_variable);
	}

	void
	print()
	{
		auto root_obj = pop.root();
		std::cout << root_obj->some_variable << std::endl;
	}
};

void
pool_example()
{
	auto foo = Foo("poolfile");
	foo.print();
	foo.set(42);
	foo.print();
	foo.increment();
	foo.print();
}
//! [pool_class_member_example]

int
main()
{
	try {
		pool_example();
	} catch (const std::exception &e) {
		std::cerr << "Exception " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
