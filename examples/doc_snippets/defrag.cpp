// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

//! [defrag_usage_example]
#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/defrag.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/atomic_base.h>

using namespace pmem::obj;

void
defrag_example()
{
	struct root {
		persistent_ptr<int> i;
		persistent_ptr<vector<int>> v;
		persistent_ptr<vector<double>> v2;
	};

	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto r = pop.root();

	persistent_ptr<int> i_ptr;
	transaction::run(pop, [&] {
		r->i = make_persistent<int>(5);
		r->v = make_persistent<vector<int>>();
		r->v2 = make_persistent<vector<double>>();

		i_ptr = make_persistent<int>(10);
	});

	r->v->push_back(15);

	/* Create a defrag object for elements in the current pool */
	defrag my_defrag(pop);
	/* And add all selected pointers for the defragmentation */
	my_defrag.add(r->i);
	/*
	 * Adding ptr<vector<T>> means also adding internal container's
	 * pointer(s), because it ('vector<int>' in this case) implements
	 * method 'for_each_ptr'.
	 */
	my_defrag.add(r->v);
	/*
	 * We can also add just the reference of an element (in this case
	 * vector<double>). This means the persistent_ptr ('r->v2' in this
	 * case) itself won't be added for the defragmentation.
	 */
	my_defrag.add(*r->v2);
	my_defrag.add(i_ptr);

	/*
	 * Out of curosity, we can check if a class of an object is
	 * defragmentable or not.
	 */
	std::cout << is_defragmentable<persistent_ptr<int>>(); /* false */
	static_assert(is_defragmentable<vector<char>>(), "should not assert");

	pobj_defrag_result result;
	try {
		/*
		 * Start when all chosen pointers are added. It can throw an
		 * error, when failed (e.g. to allocate) in any moment of the
		 * process.
		 */
		result = my_defrag.run();
	} catch (pmem::defrag_error &e) {
		std::cerr << e.what() << "No. of the relocated objects: "
			  << e.result.relocated
			  << " out of total: " << e.result.total
			  << " processed." << std::endl;
	}

	/* After successful defragmentation result contains basic summary */
	std::cout << "No. of relocated objects: " << result.relocated
		  << " out of total: " << result.total << " processed."
		  << std::endl;
}
//! [defrag_usage_example]
