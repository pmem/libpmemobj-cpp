/*
 * Copyright 2020, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
