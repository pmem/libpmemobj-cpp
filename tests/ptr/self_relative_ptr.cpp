// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * obj_cpp_ptr.c -- cpp bindings test
 *
 */

#include "ptr.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/self_relative_ptr.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <sstream>

#define LAYOUT "cpp"

template <typename T>
using self_relative_ptr = nvobj::experimental::self_relative_ptr<T>;
using self_relative_ptr_base = nvobj::experimental::self_relative_ptr_base;

namespace
{

using root = templated_root<self_relative_ptr, self_relative_ptr_base>;

/*
 * test_offset -- test offset calculation within a hierarchy
 */
void
test_offset(nvobj::pool<root> &pop)
{
	struct A {
		uint64_t a;
	};

	struct B {
		uint64_t b;
	};

	struct C : public A, public B {
		uint64_t c;
	};

	try {
		nvobj::transaction::run(pop, [] {
			auto distance =
				self_relative_ptr_base::distance_between;

			self_relative_ptr<C> cptr = nvobj::make_persistent<C>();
			self_relative_ptr<B> bptr = cptr;
			std::cout << distance(bptr, cptr) << std::endl;
			UT_ASSERT(std::abs(distance(bptr, cptr)) == sizeof(A));

			self_relative_ptr<B> bptr2;
			bptr2 = cptr;
			UT_ASSERT(std::abs(distance(bptr2, cptr)) == sizeof(A));

			self_relative_ptr<B> bptr3 =
				static_cast<self_relative_ptr<B>>(cptr);
			UT_ASSERT(std::abs(distance(bptr3, cptr)) == sizeof(A));

			nvobj::delete_persistent<C>(cptr.to_persitent_ptr());
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

void
test_base_ptr_casting(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->arr[0] = self_relative_ptr<foo>{
				nvobj::make_persistent<foo>()};
			r->arr[1] = self_relative_ptr<int>{
				nvobj::make_persistent<int>(TEST_INT)};
			r->arr[2] = nullptr;

			UT_ASSERTne(r->arr[0].to_void_pointer(), nullptr);
			UT_ASSERTeq(*static_cast<int *>(
					    r->arr[1].to_void_pointer()),
				    TEST_INT);
			UT_ASSERTeq(r->arr[2].to_void_pointer(), nullptr);

			self_relative_ptr<foo> tmp0 =
				static_cast<foo *>(r->arr[0].to_void_pointer());
			self_relative_ptr<int> tmp1 =
				static_cast<int *>(r->arr[1].to_void_pointer());
			self_relative_ptr<foo> tmp2 =
				static_cast<foo *>(r->arr[2].to_void_pointer());
			nvobj::delete_persistent<foo>(tmp0.to_persitent_ptr());
			nvobj::delete_persistent<int>(tmp1.to_persitent_ptr());
			nvobj::delete_persistent<foo>(tmp2.to_persitent_ptr());
		});
	} catch (...) {
		UT_ASSERT(0);
	}
}

} /* namespace */

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_ptr_operators_null<self_relative_ptr>();
	test_ptr_transactional(pop);
	test_ptr_array(pop);
	test_offset(pop);
	test_base_ptr_casting(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
