// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

struct Object {
	Object(int d, nvobj::string_view s) : data(d), s(s)
	{
	}

	nvobj::p<int> data;
	nvobj::experimental::inline_string s;
};

struct root {
	nvobj::persistent_ptr<Object> o1;
	nvobj::persistent_ptr<Object> o2;
};

void
test_inline_string(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	const auto req_capacity = 100;

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::standard_alloc_policy<void> allocator;
			r->o1 = static_cast<nvobj::persistent_ptr<Object>>(
				allocator.allocate(sizeof(Object) +
						   req_capacity));
			r->o2 = static_cast<nvobj::persistent_ptr<Object>>(
				allocator.allocate(sizeof(Object) +
						   req_capacity));

			new (r->o1.get()) Object(1, "abcd");
			new (r->o2.get()) Object(2, "xxxxxxx");
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(r->o1->data, 1);
	UT_ASSERTeq(r->o2->data, 2);

	UT_ASSERT(nvobj::string_view(r->o1->s).compare("abcd") == 0);
	UT_ASSERT(r->o1->s.size() == 4);
	UT_ASSERT(memcmp(r->o1->s.data(), "abcd", 4) == 0);

	UT_ASSERT(nvobj::string_view(r->o2->s).compare("xxxxxxx") == 0);

	try {
		nvobj::transaction::run(pop, [&] {
			*(r->o1) = *(r->o2);

			UT_ASSERTeq(r->o1->data, 2);
			UT_ASSERTeq(r->o2->data, 2);

			UT_ASSERT(nvobj::string_view(r->o1->s).compare(
					  "xxxxxxx") == 0);
			UT_ASSERT(nvobj::string_view(r->o2->s).compare(
					  "xxxxxxx") == 0);

			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERTeq(r->o1->data, 1);
	UT_ASSERTeq(r->o2->data, 2);

	UT_ASSERT(nvobj::string_view(r->o1->s).compare("abcd") == 0);
	UT_ASSERT(nvobj::string_view(r->o2->s).compare("xxxxxxx") == 0);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<Object>(r->o1);
		nvobj::delete_persistent<Object>(r->o2);
	});
}

void
test_ctor_exception_nopmem(nvobj::pool<struct root> &pop)
{
	try {
		Object o(1, "example");
	} catch (pmem::pool_error &) {
	} catch (...) {
		UT_ASSERT(0);
	}
}
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<struct root> pop;

	try {
		pop = nvobj::pool<root>::create(path, LAYOUT, PMEMOBJ_MIN_POOL,
						S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_inline_string(pop);
	test_ctor_exception_nopmem(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
