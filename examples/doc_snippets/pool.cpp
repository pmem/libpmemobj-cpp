// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2018, Intel Corporation */

/*
 * pool.cpp -- C++ documentation snippets.
 */

//! [pool_example]
#include <fcntl.h>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

using namespace pmem::obj;

void
pool_example()
{

	// pool root structure
	struct root {
		p<int> some_array[42];
		p<int> some_other_array[42];
		p<double> some_variable;
	};

	// create a pmemobj pool
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);

	// close a pmemobj pool
	pop.close();

	// or open a pmemobj pool
	pop = pool<root>::open("poolfile", "layout");

	// typical usage schemes
	auto root_obj = pop.root();

	// low-level memory manipulation
	root_obj->some_variable = 3.2;
	pop.persist(root_obj->some_variable);

	pop.memset_persist(root_obj->some_array, 2,
			   sizeof(root_obj->some_array));

	pop.memcpy_persist(root_obj->some_other_array, root_obj->some_array,
			   sizeof(root_obj->some_array));

	pop.close();

	// check pool consistency
	pool<root>::check("poolfile", "layout");
}
//! [pool_example]

//! [pool_base_example]
#include <fcntl.h>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>

using namespace pmem::obj;

void
pool_base_example()
{

	struct some_struct {
		p<int> some_array[42];
		p<int> some_other_array[42];
		p<int> some_variable;
	};

	// create a pmemobj pool
	auto pop = pool_base::create("poolfile", "", PMEMOBJ_MIN_POOL);

	// close a pmemobj pool
	pop.close();

	// or open a pmemobj pool
	pop = pool_base::open("poolfile", "");

	// no "root" object available in pool_base
	persistent_ptr<some_struct> pval;
	make_persistent_atomic<some_struct>(pop, pval);

	// low-level memory manipulation
	pval->some_variable = 3;
	pop.persist(pval->some_variable);

	pop.memset_persist(pval->some_array, 2, sizeof(pval->some_array));

	pop.memcpy_persist(pval->some_other_array, pval->some_array,
			   sizeof(pval->some_array));

	pop.close();

	// check pool consistency
	pool_base::check("poolfile", "");
}
//! [pool_base_example]
