// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * persistent.cpp -- C++ documentation snippets.
 */

//! [p_property_example]
#include <fcntl.h>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
p_property_example()
{
	struct compound_type {

		void
		set_some_variable(int val)
		{
			some_variable = val;
		}

		int some_variable;
		double some_other_variable;
	};

	/* pool root structure */
	static struct root {
		p<int> counter;		 /* this is OK */
		p<compound_type> whoops; /* this is hard to use */
	} proot;

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);

	/* typical usage schemes */
	transaction::run(pop, [&] {
		proot.counter = 12; /* atomic */
		/* one way to change `whoops` */
		proot.whoops.get_rw().set_some_variable(2);
		proot.whoops.get_rw().some_other_variable = 3.0;
	});

	/* Changing a p<> variable outside of a transaction is a volatile
	 * modification. No way to ensure persistence in case of power failure.
	 */
	proot.counter = 12;
}
//! [p_property_example]

//! [persistent_ptr_example]
#include <fcntl.h>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
persistent_ptr_example()
{
	struct compound_type {

		void
		set_some_variable(int val)
		{
			some_variable = val;
		}

		int some_variable;
		double some_other_variable;
	};

	/* pool root structure */
	struct root {
		persistent_ptr<compound_type> comp;
	} proot;

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);

	/* typical usage schemes */
	transaction::run(pop, [&] {
		proot.comp = make_persistent<compound_type>(); /* allocation */
		proot.comp->set_some_variable(12);     /* call function */
		proot.comp->some_other_variable = 2.3; /* set variable */
	});

	/* reading from the persistent_ptr */
	compound_type tmp = *proot.comp;
	(void)tmp;

	/* Changing a persistent_ptr<> variable outside of a transaction is a
	 * volatile modification. No way to ensure persistence in case of power
	 * failure. */
	proot.comp->some_variable = 12;
}
//! [persistent_ptr_example]

//! [persistent_ptr_casting_example]
#include <iostream>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
persistent_ptr_conversion_example()
{
	/* pool root structure */
	struct root {
		persistent_ptr<int> pfoo;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);

	/* Casting persistent_ptr to persistent_ptr_base */
	transaction::run(pop, [&] {
		/* Good: any persistent_ptr<T> can be stored in a base ptr */
		persistent_ptr_base i_ptr_base = make_persistent<int>(10);

		/*
		 * Wrong: even though raw pointer can be used to create new
		 * persistent_ptr it's not advised to use it this way, since
		 * there's no information about underlying/template type
		 */
		persistent_ptr<double> dptr = i_ptr_base.raw();
		std::cout << *dptr; // contains trash data

		/*
		 * Acceptable: it's not advised, but it will work properly.
		 * Although, you have to be sure the underlying type is correct
		 */
		persistent_ptr<int> iptr_nonbase = i_ptr_base.raw();
		std::cout << *iptr_nonbase; // contains proper data

		/*
		 * Wrong: illegal call for derived constructor.
		 * no viable conversion from 'persistent_ptr_base' to
		 * 'persistent_ptr<int>'
		 * iptr_nonbase = i_ptr_base;
		 */

		/*
		 * Wrong: conversion from base class to persistent_ptr<T> is not
		 * possible: no matching conversion from 'persistent_ptr_base'
		 * to 'persistent_ptr<int>'
		 * iptr_nonbase = static_cast<persistent_ptr<int>>(i_ptr_base);
		 */

		/* Good: you can use base and ptr classes with volatile pointer
		 */
		persistent_ptr<int> i_ptr = make_persistent<int>(10);
		persistent_ptr_base *i_ptr_ref = &i_ptr;
		std::cout << i_ptr_ref->raw().off; /* contains PMEMoid's data */
	});

	struct A {
		uint64_t a;
	};
	struct B {
		uint64_t b;
	};
	struct C : public A, public B {
		uint64_t c;
	};

	/* Convertible types, using struct A, B and C */
	transaction::run(pop, [] {
		/* Good: conversion from type C to B, using copy constructor */
		auto cptr = make_persistent<C>();
		persistent_ptr<B> bptr = cptr;
		std::cout << (bptr->b ==
			      cptr->b); /* thanks to conversion and */
					/* recalculating offsets it's correct */

		/*
		 * Good: conversion from type C to B, using converting
		 * assignment operator
		 */
		persistent_ptr<B> bptr2;
		bptr2 = cptr;
		std::cout << (bptr2->b == cptr->b); /* true */

		/* Good: direct conversion using static_cast */
		persistent_ptr<B> bptr3 = static_cast<persistent_ptr<B>>(cptr);
		std::cout << (bptr3->b == cptr->b); /* true */

		/*
		 * Wrong: conversion to base class and then to different ptr.
		 * class no matching conversion from 'persistent_ptr_base' to
		 * 'persistent_ptr<B>'
		 */
		/*
		 * auto cptr2 = (persistent_ptr_base)cptr;
		 * persistent_ptr<B> bptr4 =
		 *		static_cast<persistent_ptr<B>>(cptr2);
		 */

		/*
		 * Wrong: conversion to 'void *' and then to different ptr
		 * class. ambiguous conversion from 'void *' to
		 * 'persistent_ptr<B>'
		 */
		/*
		 * auto cptr3 = (void *)&cptr;
		 * persistent_ptr<B> bptr5 =
		 *		static_cast<persistent_ptr<B>>(cptr3);
		 */
	});
}
//! [persistent_ptr_casting_example]

int
main()
{
	try {
		p_property_example();
		persistent_ptr_example();
		persistent_ptr_conversion_example();
	} catch (const std::exception &e) {
		std::cerr << "Exception " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
