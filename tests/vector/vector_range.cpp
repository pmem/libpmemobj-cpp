// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmemobj = pmem::obj;

using vec_type = container_t<int>;

struct root {
	pmemobj::persistent_ptr<vec_type> pptr;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = pmemobj::pool<root>::create(
		path, "VectorTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	auto r = pop.root();

	try {
		pmemobj::transaction::run(pop, [&] {
			r->pptr = pmemobj::make_persistent<vec_type>(10U, 1);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	vec_type &pmem_vec = *(r->pptr);
	const vec_type &const_pmem_vec = *(r->pptr);

	/* test std::out_of_range exceptions */
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice =
				pmem_vec.range(0, 10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice =
				pmem_vec.crange(0, 10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmemobj::transaction::run(pop, [&] {
			pmem::obj::slice<vec_type::const_iterator> slice =
				const_pmem_vec.range(0,
						     10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice =
				pmem_vec.range(0, 10, 3); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	bool exception_thrown = false;
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice = pmem_vec.range(0, 11); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice =
				pmem_vec.range(0, 11, 3); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		pmemobj::transaction::run(pop, [&] {
			pmem::obj::slice<vec_type::const_iterator> slice =
				const_pmem_vec.range(0, 11); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice = pmem_vec.crange(0, 11); /* should throw */
			(void)slice;
		});
		UT_ASSERT(0);
	} catch (std::out_of_range &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);

	/* test returned values */
	try {
		pmemobj::transaction::run(pop, [&] {
			auto slice1 = pmem_vec.range(0, 3);

			UT_ASSERTeq(&pmem_vec.front(), slice1.begin());
			UT_ASSERTeq(&pmem_vec.front() + 3, slice1.end());

			auto slice2 = pmem_vec.range(0, 3, 1);

			UT_ASSERTeq(&pmem_vec.front(), &*slice2.begin());
			UT_ASSERTeq(&pmem_vec.front() + 3, &*slice2.end());

			auto slice3 = pmem_vec.range(0, 10, 11);

			UT_ASSERTeq(&pmem_vec.front(), &*slice3.begin());
			UT_ASSERTeq(&pmem_vec.front() + 10, &*slice3.end());

			pmem::obj::slice<vec_type::const_iterator> slice4 =
				const_pmem_vec.range(0, 3);

			UT_ASSERTeq(&const_pmem_vec.front(), slice4.begin());
			UT_ASSERTeq(&const_pmem_vec.front() + 3, slice4.end());

			auto slice5 = pmem_vec.crange(0, 3);

			UT_ASSERTeq(&pmem_vec.front(), slice5.begin());
			UT_ASSERTeq(&pmem_vec.front() + 3, slice5.end());

			pmemobj::delete_persistent<vec_type>(r->pptr);
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
