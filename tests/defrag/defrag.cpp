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

#include "unittest.hpp"

#include <libpmemobj++/defrag.hpp>
#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj/atomic_base.h>

namespace nvobj = pmem::obj;

namespace
{

struct root {
	nvobj::persistent_ptr<int> i;
	nvobj::persistent_ptr<char> c;
	nvobj::persistent_ptr<double> d;
};

void
test_basic(nvobj::pool<root> &pop)
{
	nvobj::transaction::run(pop, [&] {
		pop.root()->i = nvobj::make_persistent<int>(5);
		pop.root()->c = nvobj::make_persistent<char>('a');
		pop.root()->d = nvobj::make_persistent<double>(10);
	});

	UT_ASSERT(!nvobj::is_defragmentable<nvobj::persistent_ptr<double>>());

	nvobj::defrag my_defrag(pop);
	my_defrag.add(pop.root()->i);
	my_defrag.add(pop.root()->c);
	my_defrag.add(pop.root()->d);

	pobj_defrag_result res;
	try {
		res = my_defrag.run();
	} catch (pmem::defrag_error &) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(res.total, 3);
}

/*
 * Non-defragmentable types added for the defragmentation
 * should not increase the total number of objects.
 */
void
test_add_empty(nvobj::pool<root> &pop)
{
	nvobj::persistent_ptr<double> d;
	nvobj::transaction::run(pop, [&] {
		pop.root()->i = nvobj::make_persistent<int>(5);
		pop.root()->c = nvobj::make_persistent<char>('a');

		d = nvobj::make_persistent<double>(10);
	});

	UT_ASSERT(!nvobj::is_defragmentable<nvobj::persistent_ptr<double>>());
	UT_ASSERT(!nvobj::is_defragmentable<double>());

	nvobj::defrag my_defrag(pop);
	my_defrag.add(*pop.root()->i);
	my_defrag.add(*pop.root()->c);
	my_defrag.add(*d);

	pobj_defrag_result res;
	try {
		res = my_defrag.run();
	} catch (pmem::defrag_error &) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(res.total, 0);

	nvobj::transaction::run(pop,
				[&] { nvobj::delete_persistent<double>(d); });
}

/*
 * When trying to add any object from outside of the selected pool,
 * the 'std::runtime_error' exception should be thrown.
 */
void
test_try_add_wrong_pointer(nvobj::pool<root> &pop, std::string path)
{
	nvobj::persistent_ptr<double> d;
	nvobj::pool<root> pop_test;

	try {
		pop_test = nvobj::pool<struct root>::create(
			path, "layout", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path.c_str());
	}

	nvobj::transaction::run(pop_test, [&] {
		pop_test.root()->i = nvobj::make_persistent<int>(1);

		d = nvobj::make_persistent<double>(10);
	});

	nvobj::defrag my_defrag(pop);
	try {
		my_defrag.add(pop_test.root()->i);
		UT_ASSERT(0);
	} catch (std::runtime_error &e) {
		UT_ASSERT(e.what() != std::string());
	} catch (...) {
		UT_ASSERT(0);
	}
	try {
		my_defrag.add(d);
		UT_ASSERT(0);
	} catch (std::runtime_error &e) {
		UT_ASSERT(e.what() != std::string());
	} catch (...) {
		UT_ASSERT(0);
	}
	try {
		my_defrag.add(*d);
		UT_ASSERT(0);
	} catch (std::runtime_error &e) {
		UT_ASSERT(e.what() != std::string());
	} catch (...) {
		UT_ASSERT(0);
	}

	pobj_defrag_result res;
	try {
		res = my_defrag.run();
	} catch (pmem::defrag_error &) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(res.total, 0);

	nvobj::transaction::run(pop_test,
				[&] { nvobj::delete_persistent<double>(d); });
	pop_test.close();
}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<struct root>::create(
			path, "layout", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_basic(pop);
	test_add_empty(pop);
	test_try_add_wrong_pointer(pop, std::string(path) + "_tmp");

	pop.close();

	return 0;
}
