// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * make_persistent.cpp -- C++ documentation snippets.
 */

#include <iostream>
//! [make_example]
#include <fcntl.h>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
make_persistent_example()
{
	struct compound_type {

		compound_type(int val, double dval)
		    : some_variable(val), some_other_variable(dval)
		{
		}

		void
		set_some_variable(int val)
		{
			some_variable = val;
		}

		p<int> some_variable;
		p<double> some_other_variable;
	};

	/* pool root structure */
	struct root {
		persistent_ptr<compound_type> comp;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	/* typical usage schemes */
	transaction::run(pop, [&] {
		/* allocation with constructor argument passing */
		proot->comp = make_persistent<compound_type>(1, 2.0);

		/* transactionally delete the object, ~compound_type() is called
		 */
		delete_persistent<compound_type>(proot->comp);

		/* set pointer to null so that after restart it's known whether
		 * compound_type is still allocated or not */
		proot->comp = nullptr;
	});

	/* throws an transaction_scope_error exception */
	auto arr1 = make_persistent<compound_type>(2, 15.0);
	delete_persistent<compound_type>(arr1);
}
//! [make_example]

//! [make_array_example]
#include <fcntl.h>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
make_persistent_array_example()
{
	struct compound_type {

		compound_type() : some_variable(0), some_other_variable(0)
		{
		}

		void
		set_some_variable(int val)
		{
			some_variable = val;
		}

		p<int> some_variable;
		p<double> some_other_variable;
	};

	/* pool root structure */
	struct root {
		persistent_ptr<compound_type[]> comp;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	/* typical usage schemes */
	transaction::run(pop, [&] {
		/* allocate an array of 20 objects - compound_type must be
		 * default constructible */
		proot->comp = make_persistent<compound_type[]>(20);
		/* another allocation method */
		auto arr1 = make_persistent<compound_type[3]>();

		/* transactionally delete arrays , ~compound_type() is called */
		delete_persistent<compound_type[]>(proot->comp, 20);
		delete_persistent<compound_type[3]>(arr1);

		/* set pointer to null so that after restart it's known whether
		 * compound_type is still allocated or not */
		proot->comp = nullptr;
	});

	/* throws an transaction_scope_error exception */
	auto arr1 = make_persistent<compound_type[3]>();
	delete_persistent<compound_type[3]>(arr1);
}
//! [make_array_example]

//! [make_atomic_example]
#include <fcntl.h>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
make_persistent_atomic_example()
{
	struct compound_type {

		compound_type(int val, double dval)
		    : some_variable(val), some_other_variable(dval)
		{
		}

		void
		set_some_variable(int val)
		{
			some_variable = val;
		}

		p<int> some_variable;
		p<double> some_other_variable;
	};

	/* pool root structure */
	struct root {
		persistent_ptr<compound_type> comp;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	/* typical usage schemes */

	/* atomic allocation and construction with arguments passing */
	make_persistent_atomic<compound_type>(pop, proot->comp, 1, 2.0);

	/* atomic object deallocation, ~compound_type() is not called */
	delete_persistent<compound_type>(proot->comp);

	/* error prone cases */
	transaction::run(pop, [&] {
		/* possible invalid state in case of transaction abort */
		make_persistent_atomic<compound_type>(pop, proot->comp, 1, 1.3);
		delete_persistent_atomic<compound_type>(proot->comp);
	});
}
//! [make_atomic_example]

//! [make_array_atomic_example]
#include <fcntl.h>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
make_persistent_array_atomic_example()
{
	struct compound_type {

		compound_type() : some_variable(0), some_other_variable(0)
		{
		}

		void
		set_some_variable(int val)
		{
			some_variable = val;
		}

		p<int> some_variable;
		p<double> some_other_variable;
	};

	/* pool root structure */
	struct root {
		persistent_ptr<compound_type[]> comp;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	/* typical usage schemes */

	/* atomic array allocation and construction - the compound_type has to
	 * be default constructible */
	make_persistent_atomic<compound_type[]>(pop, proot->comp, 20);

	persistent_ptr<compound_type[42]> arr;
	make_persistent_atomic<compound_type[42]>(pop, arr);

	/* atomic array deallocation, no destructor being called */
	delete_persistent_atomic<compound_type[]>(proot->comp, 20);
	delete_persistent_atomic<compound_type[42]>(arr);

	/* error prone cases */
	transaction::run(pop, [&] {
		/* possible invalid state in case of transaction abort */
		make_persistent_atomic<compound_type[]>(pop, proot->comp, 30);
		delete_persistent_atomic<compound_type[]>(proot->comp, 30);
	});
}
//! [make_array_atomic_example]

int
main()
{
	try {
		make_persistent_example();
		make_persistent_array_example();
		make_persistent_atomic_example();
		make_persistent_array_atomic_example();
	} catch (const std::exception &e) {
		std::cerr << "Exception " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
