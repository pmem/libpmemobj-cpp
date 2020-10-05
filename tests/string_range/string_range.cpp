// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "../common/unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/transaction.hpp>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> s;
};

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "StringTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->s = nvobj::make_persistent<S>(10U, 'a');
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	S &str = *(r->s);
	const S &const_str = *(r->s);

	/* test std::out_of_range exceptions */
	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.range(0, 10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.crange(0, 10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			pmem::obj::slice<S::const_iterator> slice =
				const_str.range(0, 10); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.range(0, 10, 3); /* should not throw */
			(void)slice;
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			auto slice = str.range(0, 11); /* should throw */
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
		nvobj::transaction::run(pop, [&] {
			auto slice = str.range(0, 11, 3); /* should throw */
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
		nvobj::transaction::run(pop, [&] {
			pmem::obj::slice<S::const_iterator> slice =
				const_str.range(0, 11); /* should throw */
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
		nvobj::transaction::run(pop, [&] {
			auto slice = str.crange(0, 11); /* should throw */
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
		nvobj::transaction::run(pop, [&] {
			auto slice1 = str.range(0, 3);

			UT_ASSERTeq(&str.front(), slice1.begin());
			UT_ASSERTeq(&str.front() + 3, slice1.end());

			auto slice2 = str.range(0, 3, 1);

			UT_ASSERTeq(&str.front(), &*slice2.begin());
			UT_ASSERTeq(&str.front() + 3, &*slice2.end());

			auto slice3 = str.range(0, 10, 11);

			UT_ASSERTeq(&str.front(), &*slice3.begin());
			UT_ASSERTeq(&str.front() + 10, &*slice3.end());

			pmem::obj::slice<S::const_iterator> slice4 =
				const_str.range(0, 3);

			UT_ASSERTeq(&const_str.front(), slice4.begin());
			UT_ASSERTeq(&const_str.front() + 3, slice4.end());

			auto slice5 = str.crange(0, 3);

			UT_ASSERTeq(&str.front(), slice5.begin());
			UT_ASSERTeq(&str.front() + 3, slice5.end());

			nvobj::delete_persistent<S>(r->s);
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
