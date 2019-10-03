//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
#include <libpmemobj++/make_persistent.hpp>

namespace nvobj = pmem::obj;
using vector_type = container_t<int>;

struct root {
	nvobj::persistent_ptr<vector_type> v1;
	nvobj::persistent_ptr<vector_type> v2;
};

template <class C>
void
test_copy_ctor_01(nvobj::pool<struct root> &pop, const C &x)
{
	typename C::size_type s = x.size();

	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v2 = nvobj::make_persistent<C>(x); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v2->size() == s);
	UT_ASSERT(*(r->v2) == x);

	try {
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v2); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

template <class C>
void
test_copy_ctor_02(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(
			pop, [&] { r->v1 = nvobj::make_persistent<C>(3U, 2); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::persistent_ptr<C> v3 =
				nvobj::make_persistent<C>(*(r->v1));
			UT_ASSERT(*v3 == *(r->v1));
			nvobj::delete_persistent<C>(v3);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(
			pop, [&] { nvobj::delete_persistent<C>(r->v1); });
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc < 2) {
		std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
		return 1;
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(path, "VectorTest: copy.pass",
					     PMEMOBJ_MIN_POOL * 2,
					     S_IWUSR | S_IRUSR);

	auto r = pop.root();
	{
		int a[] = {0, 1, 2, 3, 4, 5, 6, 7, 8,
			   9, 8, 7, 6, 5, 4, 3, 1, 0};
		int *an = a + sizeof(a) / sizeof(a[0]);

		try {
			nvobj::transaction::run(pop, [&] {
				r->v1 = nvobj::make_persistent<vector_type>(a,
									    an);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}

		test_copy_ctor_01(pop, *(r->v1));

		try {
			nvobj::transaction::run(pop, [&] {
				nvobj::delete_persistent<vector_type>(r->v1);
			});
		} catch (std::exception &e) {
			UT_FATALexc(e);
		}
	}
	{
		test_copy_ctor_02<vector_type>(pop);
	}

	pop.close();

	return 0;
}
