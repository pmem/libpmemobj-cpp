/*
 * Copyright 2016-2019, Intel Corporation
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

/*
 * obj_cpp_make_persistent_atomic.cpp -- cpp make_persistent_atomic test for
 * objects
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/ctl.h>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

const int TEST_ARR_SIZE = 10;

class foo {
public:
	foo() : bar(1)
	{
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			this->arr[i] = 1;
	}

	foo(const int &val) : bar(val)
	{
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			this->arr[i] = static_cast<char>(val);
	}

	foo(int &val, char arr_val) : bar(val)
	{
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			this->arr[i] = arr_val;
	}

	/*
	 * Assert values of foo.
	 */
	void
	check_foo(int val, char arr_val)
	{
		UT_ASSERTeq(val, this->bar);
		for (int i = 0; i < TEST_ARR_SIZE; ++i)
			UT_ASSERTeq(arr_val, this->arr[i]);
	}

	nvobj::p<int> bar;
	nvobj::p<char> arr[TEST_ARR_SIZE];
};

struct root {
	nvobj::persistent_ptr<foo> pfoo;
};

/*
 * test_make_no_args -- (internal) test make_persistent without arguments
 */
void
test_make_no_args(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

	UT_ASSERT(r->pfoo == nullptr);

	nvobj::make_persistent_atomic<foo>(pop, r->pfoo);
	r->pfoo->check_foo(1, 1);

	nvobj::delete_persistent_atomic<foo>(r->pfoo);
}

/*
 * test_make_args -- (internal) test make_persistent with arguments
 */
void
test_make_args(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();
	UT_ASSERT(r->pfoo == nullptr);

	nvobj::make_persistent_atomic<foo>(pop, r->pfoo, 2);
	r->pfoo->check_foo(2, 2);

	nvobj::delete_persistent_atomic<foo>(r->pfoo);

	nvobj::make_persistent_atomic<foo>(pop, r->pfoo, 3, 4);
	r->pfoo->check_foo(3, 4);

	nvobj::delete_persistent_atomic<foo>(r->pfoo);
}

/*
 * test_delete_null -- (internal) test atomic delete nullptr
 */
void
test_delete_null(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<foo> pfoo;

	UT_ASSERT(pfoo == nullptr);

	try {
		nvobj::delete_persistent_atomic<foo>(pfoo);
	} catch (...) {
		UT_ASSERT(0);
	}
}

/*
 * test_flags -- (internal) test proper handling of flags
 */
void
test_flags(nvobj::pool<struct root> &pop)
{
	nvobj::persistent_ptr<root> r = pop.root();

	auto alloc_class = pop.ctl_set<struct pobj_alloc_class_desc>(
		"heap.alloc_class.new.desc",
		{sizeof(foo) + 16, 0, 200, POBJ_HEADER_COMPACT, 0});

	try {
		nvobj::make_persistent_atomic<foo>(
			pop, r->pfoo,
			nvobj::allocation_flag_atomic::class_id(
				alloc_class.class_id));
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(pmemobj_alloc_usable_size(r->pfoo.raw()), sizeof(foo));

	try {
		nvobj::delete_persistent_atomic<foo>(r->pfoo);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::make_persistent_atomic<foo>(
			pop, r->pfoo,
			nvobj::allocation_flag_atomic::class_id(
				alloc_class.class_id),
			1, 2);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	r->pfoo->check_foo(1, 2);

	UT_ASSERTeq(pmemobj_alloc_usable_size(r->pfoo.raw()), sizeof(foo));

	try {
		nvobj::delete_persistent_atomic<foo>(r->pfoo);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<struct root> pop;

	try {
		pop = nvobj::pool<struct root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_make_no_args(pop);
	test_make_args(pop);
	test_delete_null(pop);
	test_flags(pop);

	pop.close();

	return 0;
}
