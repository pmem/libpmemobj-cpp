// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/vector.hpp>
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
	nvobj::persistent_ptr<nvobj::vector<int>> vi;
	nvobj::persistent_ptr<nvobj::vector<double>> vd;
};

void
test_vector_basic(nvobj::pool<root> &pop)
{
	nvobj::transaction::run(pop, [&] {
		pop.root()->vi = nvobj::make_persistent<nvobj::vector<int>>();
		pop.root()->vd =
			nvobj::make_persistent<nvobj::vector<double>>();
	});

	static_assert(!nvobj::is_defragmentable<
			      nvobj::persistent_ptr<nvobj::vector<int>>>(),
		      "should not assert");
	static_assert(nvobj::is_defragmentable<nvobj::vector<int>>(),
		      "should not assert");

	pop.root()->vi->push_back(5);
	pop.root()->vi->push_back(10);
	pop.root()->vi->push_back(15);

	pop.root()->vd->push_back(1);

	nvobj::defrag my_defrag(pop);
	my_defrag.add(pop.root()->vi);
	my_defrag.add(pop.root()->vd);

	pobj_defrag_result res;
	try {
		res = my_defrag.run();
	} catch (pmem::defrag_error &) {
		UT_ASSERT(0);
	}

	/* 2 pointers + 2 vector objects (each have only 1 internal pointer) */
	UT_ASSERTeq(res.total, 4);
}

/*
 * Adding only objects pointed by ptrs, without pointers themselves.
 * One of the vectors is empty, hence finally only 1 object was added
 * to the defragmentation.
 */
void
test_vector_add_no_ptrs(nvobj::pool<root> &pop)
{
	nvobj::transaction::run(pop, [&] {
		pop.root()->vi = nvobj::make_persistent<nvobj::vector<int>>();
		pop.root()->vd =
			nvobj::make_persistent<nvobj::vector<double>>();
	});

	pop.root()->vi->push_back(5);

	nvobj::defrag my_defrag(pop);
	my_defrag.add(*pop.root()->vi);
	my_defrag.add(*pop.root()->vd);

	pobj_defrag_result res;
	try {
		res = my_defrag.run();
	} catch (pmem::defrag_error &) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(res.total, 1);
}

/*
 * When trying to add any object from outside of the selected pool,
 * the 'std::runtime_error' exception should be thrown.
 */
void
test_vector_try_add_wrong_pointer(nvobj::pool<root> &pop, std::string path)
{
	nvobj::persistent_ptr<nvobj::vector<char>> vc;
	nvobj::pool<root> pop_test;

	try {
		pop_test = nvobj::pool<struct root>::create(
			path, "layout", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path.c_str());
	}

	nvobj::transaction::run(pop_test, [&] {
		pop_test.root()->vi =
			nvobj::make_persistent<nvobj::vector<int>>();

		vc = nvobj::make_persistent<nvobj::vector<char>>();
	});

	nvobj::defrag my_defrag(pop);
	try {
		my_defrag.add(pop_test.root()->vi);
		UT_ASSERT(0);
	} catch (std::runtime_error &e) {
		UT_ASSERT(e.what() != std::string());
	} catch (...) {
		UT_ASSERT(0);
	}
	try {
		my_defrag.add(vc);
		UT_ASSERT(0);
	} catch (std::runtime_error &e) {
		UT_ASSERT(e.what() != std::string());
	} catch (...) {
		UT_ASSERT(0);
	}
	try {
		my_defrag.add(*vc);
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

	nvobj::transaction::run(pop_test, [&] {
		nvobj::delete_persistent<nvobj::vector<char>>(vc);
	});
	pop_test.close();
}
}

static void
test(int argc, char *argv[])
{
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

	test_vector_basic(pop);
	test_vector_add_no_ptrs(pop);
	test_vector_try_add_wrong_pointer(pop, std::string(path) + "_tmp");

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
