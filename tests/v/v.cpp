/*
 * Copyright 2018, Intel Corporation
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
 * obj_cpp_v.c -- cpp bindings test
 *
 */

#include "unittest.hpp"

#include <atomic>
#include <libpmemobj++/experimental/v.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;
namespace nvobj_exp = pmem::obj::experimental;

namespace
{

static const int TEST_VALUE = 10;

struct foo {
	foo() : counter(TEST_VALUE){};
	int counter;
};

struct bar {
	nvobj_exp::v<foo> vfoo;

	nvobj_exp::v<int> vi;
	nvobj_exp::v<int> vi2;
	nvobj_exp::v<char> vc;

	bar()
	{
	}
};

struct root {
	nvobj_exp::v<foo> f;
	nvobj::persistent_ptr<bar> bar_ptr;
};

/*
 * test_init -- test volatile value initialization
 */
void
test_init(nvobj::pool<root> &pop)
{
	UT_ASSERTeq(pop.root()->f.get().counter, TEST_VALUE);
	UT_ASSERTeq(pop.root()->bar_ptr->vfoo.get().counter, TEST_VALUE);
}

/*
 * test_conversion -- test v conversion operator
 */
void
test_conversion(nvobj::pool<root> &pop)
{
	auto r = pop.root()->bar_ptr;

	r->vi = 2;
	r->vc = 2;

	UT_ASSERT(r->vi == r->vc);
	UT_ASSERT(r->vi == 2);
	UT_ASSERT(2 == r->vi);
	UT_ASSERT(r->vi - 2 == 0);

	int &i1 = r->vi;
	char &i2 = r->vc;

	UT_ASSERT(i1 == i2);
	i1 = 1;

	UT_ASSERT(r->vi == i1);
}

/*
 * test_operators-- test v assignment operators
 */
void
test_operators(nvobj::pool<root> &pop)
{
	auto r = pop.root()->bar_ptr;

	r->vi = 2;
	r->vc = 3;

	UT_ASSERT(r->vi != r->vc);
	r->vi = r->vc;
	UT_ASSERT(r->vi == r->vc);

	r->vi = 2;
	r->vi2 = 3;
	std::swap(r->vi, r->vi2);
	UT_ASSERT(r->vi == 3);
	UT_ASSERT(r->vi2 == 2);

	r->vi2 = 2;
	r->vi = r->vi2;
	UT_ASSERT(r->vi == 2);
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
			path, LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	nvobj::make_persistent_atomic<bar>(pop, pop.root()->bar_ptr);

	test_init(pop);

	pop.root()->f.get().counter = 20;
	UT_ASSERTeq(pop.root()->f.get().counter, 20);

	pop.close();

	pop = nvobj::pool<struct root>::open(path, LAYOUT);

	test_init(pop);
	test_conversion(pop);
	test_operators(pop);

	pop.close();

	return 0;
}
