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

	using pool_type = pmem::obj::pool<persistent_data>;
	/* pool object */
	pool_type pop;

	const char *layout = "pool_layout";

public:
	Foo(const char *poolfile_path)
	{
		/* Create a pmemobj pool. */
		if (pool_type::check(poolfile_path, layout) == 1) {
			pop = pool_type::open(poolfile_path, layout);
		} else {
			std::cerr << "Cannot open pool" << std::endl
				  << "Trying to create a new one " << std::endl;
			pop = pool_type::create("poolfile", layout,
						PMEMOBJ_MIN_POOL);
		}
	}

	Foo() = delete;

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
		set(pop.root()->some_variable + 1);
	}

	void
	print()
	{
		std::cout << pop.root()->some_variable << std::endl;
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
