// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2020, Intel Corporation */

/*
 * obj_cpp_p_ext.c -- cpp p<> property operators test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

#include <cmath>
#include <sstream>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;

namespace
{

struct foo {
	nvobj::p<int> pint;
	nvobj::p<long long> pllong;
	nvobj::p<unsigned char> puchar;
};

struct bar {
	nvobj::p<double> pdouble;
	nvobj::p<float> pfloat;
};

struct root {
	nvobj::persistent_ptr<bar> bar_ptr;
	nvobj::persistent_ptr<foo> foo_ptr;
};

/*
 * init_foobar -- (internal) initialize the root object with specific values
 */
nvobj::persistent_ptr<root>
init_foobar(nvobj::pool_base &pop)
{
	nvobj::pool<struct root> &root_pop =
		dynamic_cast<nvobj::pool<struct root> &>(pop);
	nvobj::persistent_ptr<root> r = root_pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->bar_ptr == nullptr);
			UT_ASSERT(r->foo_ptr == nullptr);

			r->bar_ptr = pmemobj_tx_alloc(sizeof(bar), 0);
			r->foo_ptr = pmemobj_tx_alloc(sizeof(foo), 0);

			r->bar_ptr->pdouble = 1.0;
			r->bar_ptr->pfloat = 2.0;

			r->foo_ptr->puchar = 0;
			r->foo_ptr->pint = 1;
			r->foo_ptr->pllong = 2;
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	return r;
}

/*
 * cleanup_foobar -- (internal) deallocate and zero out root fields
 */
void
cleanup_foobar(nvobj::pool_base &pop)
{
	nvobj::pool<struct root> &root_pop =
		dynamic_cast<nvobj::pool<struct root> &>(pop);
	nvobj::persistent_ptr<root> r = root_pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->bar_ptr != nullptr);
			UT_ASSERT(r->foo_ptr != nullptr);

			pmemobj_tx_free(r->bar_ptr.raw());
			r->bar_ptr = nullptr;
			pmemobj_tx_free(r->foo_ptr.raw());
			r->foo_ptr = nullptr;
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->bar_ptr == nullptr);
	UT_ASSERT(r->foo_ptr == nullptr);
}

/*
 * arithmetic_test -- (internal) perform basic arithmetic tests on p<>
 */
void
arithmetic_test(nvobj::pool_base &pop)
{
	nvobj::persistent_ptr<root> r = init_foobar(pop);

	/* operations test */
	try {
		nvobj::transaction::run(pop, [&] {
			/* addition */
			r->foo_ptr->puchar += r->foo_ptr->puchar;
			r->foo_ptr->puchar +=
				static_cast<unsigned char>(r->foo_ptr->pint);
			r->foo_ptr->puchar += 2;
			UT_ASSERTeq(r->foo_ptr->puchar, 3);

			r->foo_ptr->pint =
				r->foo_ptr->pint + r->foo_ptr->puchar;
			r->foo_ptr->pint = r->foo_ptr->pint + r->foo_ptr->pint;
			r->foo_ptr->pint =
				static_cast<int>(r->foo_ptr->pllong + 8);
			UT_ASSERTeq(r->foo_ptr->pint, 10);

			/* for float assertions */
			float epsilon = 0.001F;

			/* subtraction */
			r->bar_ptr->pdouble -= r->foo_ptr->puchar;
			r->bar_ptr->pfloat -= 2;
			UT_ASSERT(std::fabs(r->bar_ptr->pdouble + 2) < epsilon);
			UT_ASSERT(std::fabs(r->bar_ptr->pfloat) < epsilon);

			r->bar_ptr->pfloat = static_cast<float>(
				r->bar_ptr->pfloat - r->bar_ptr->pdouble);
			r->bar_ptr->pdouble =
				r->bar_ptr->pdouble - r->bar_ptr->pfloat;
			UT_ASSERT(std::fabs(r->bar_ptr->pfloat - 2) < epsilon);
			UT_ASSERT(std::fabs(r->bar_ptr->pdouble + 4) < epsilon);

			/* multiplication */
			r->foo_ptr->puchar *= r->foo_ptr->puchar;
			r->foo_ptr->puchar *=
				static_cast<unsigned char>(r->foo_ptr->pint);
			r->foo_ptr->puchar *=
				static_cast<unsigned char>(r->foo_ptr->pllong);
			UT_ASSERTeq(r->foo_ptr->puchar, 180);

			r->foo_ptr->pint =
				r->foo_ptr->pint * r->foo_ptr->puchar;
			r->foo_ptr->pint = r->foo_ptr->pint * r->foo_ptr->pint;
			r->foo_ptr->pint = static_cast<int>(r->foo_ptr->pllong *
							    r->foo_ptr->pint);
			/* no assertions needed at this point */

			/* division */
			r->bar_ptr->pdouble /= r->foo_ptr->puchar;
			r->bar_ptr->pfloat /= r->foo_ptr->pllong;
			/* no assertions needed at this point */

			r->bar_ptr->pfloat = static_cast<float>(
				r->bar_ptr->pfloat / r->bar_ptr->pdouble);
			r->bar_ptr->pdouble =
				r->bar_ptr->pdouble / r->bar_ptr->pfloat;
			/* no assertions needed at this point */

			/* prefix */
			++r->foo_ptr->pllong;
			--r->foo_ptr->pllong;
			UT_ASSERTeq(r->foo_ptr->pllong, 2);

			/* postfix */
			r->foo_ptr->pllong++;
			r->foo_ptr->pllong--;
			UT_ASSERTeq(r->foo_ptr->pllong, 2);

			/* modulo */
			r->foo_ptr->pllong = 12;
			r->foo_ptr->pllong %= 7;
			UT_ASSERTeq(r->foo_ptr->pllong, 5);
			r->foo_ptr->pllong = r->foo_ptr->pllong % 3;
			UT_ASSERTeq(r->foo_ptr->pllong, 2);
			r->foo_ptr->pllong =
				r->foo_ptr->pllong % r->foo_ptr->pllong;
			UT_ASSERTeq(r->foo_ptr->pllong, 0);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	cleanup_foobar(pop);
}

/*
 * bitwise_test -- (internal) perform basic bitwise operator tests on p<>
 */
void
bitwise_test(nvobj::pool_base &pop)
{
	nvobj::persistent_ptr<root> r = init_foobar(pop);

	try {
		nvobj::transaction::run(pop, [&] {
			/* OR */
			r->foo_ptr->puchar |= r->foo_ptr->pllong;
			r->foo_ptr->puchar |= r->foo_ptr->pint;
			r->foo_ptr->puchar |= 4;
			UT_ASSERTeq(r->foo_ptr->puchar, 7);

			r->foo_ptr->pint =
				r->foo_ptr->pint | r->foo_ptr->puchar;
			r->foo_ptr->pint = r->foo_ptr->pint | r->foo_ptr->pint;
			r->foo_ptr->pint =
				static_cast<int>(r->foo_ptr->pllong | 0xF);
			UT_ASSERTeq(r->foo_ptr->pint, 15);

			/* AND */
			r->foo_ptr->puchar &= r->foo_ptr->puchar;
			r->foo_ptr->puchar &= r->foo_ptr->pint;
			r->foo_ptr->puchar &= 2;
			UT_ASSERTeq(r->foo_ptr->puchar, 2);

			r->foo_ptr->pint =
				r->foo_ptr->pint & r->foo_ptr->puchar;
			r->foo_ptr->pint = r->foo_ptr->pint & r->foo_ptr->pint;
			r->foo_ptr->pint = r->foo_ptr->pllong & 8;
			UT_ASSERTeq(r->foo_ptr->pint, 0);

			/* XOR */
			r->foo_ptr->puchar ^= r->foo_ptr->puchar;
			r->foo_ptr->puchar ^= r->foo_ptr->pint;
			r->foo_ptr->puchar ^= 2;
			UT_ASSERTeq(r->foo_ptr->puchar, 2);

			r->foo_ptr->pint =
				r->foo_ptr->pint ^ r->foo_ptr->puchar;
			r->foo_ptr->pint = r->foo_ptr->pint ^ r->foo_ptr->pint;
			r->foo_ptr->pint =
				static_cast<int>(r->foo_ptr->pllong ^ 8);
			UT_ASSERTeq(r->foo_ptr->pint, 10);

			/* RSHIFT */
			r->foo_ptr->puchar = 255;
			r->foo_ptr->puchar >>= 1;
			r->foo_ptr->puchar >>= r->foo_ptr->puchar;
			r->foo_ptr->puchar = static_cast<unsigned char>(
				r->foo_ptr->pllong >> 2);
			r->foo_ptr->puchar = static_cast<unsigned char>(
				r->foo_ptr->pllong >> r->foo_ptr->pllong);
			UT_ASSERTeq(r->foo_ptr->puchar, 0);

			/* LSHIFT */
			r->foo_ptr->puchar = 1;
			r->foo_ptr->puchar <<= 1;
			r->foo_ptr->puchar <<= r->foo_ptr->puchar;
			r->foo_ptr->puchar = static_cast<unsigned char>(
				r->foo_ptr->pllong << 2);
			r->foo_ptr->puchar = static_cast<unsigned char>(
				r->foo_ptr->pllong << r->foo_ptr->pllong);
			UT_ASSERTeq(r->foo_ptr->puchar, 8);

			/* COMPLEMENT */
			r->foo_ptr->pint = 1;
			UT_ASSERTeq(~r->foo_ptr->pint, ~1);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	cleanup_foobar(pop);
}

/*
 * stream_test -- (internal) perform basic istream/ostream operator tests on p<>
 */
void
stream_test(nvobj::pool_base &pop)
{
	nvobj::persistent_ptr<root> r = init_foobar(pop);

	try {
		nvobj::transaction::run(pop, [&] {
			std::stringstream stream("12.4");
			stream >> r->bar_ptr->pdouble;
			/*
			 * clear the stream's EOF,
			 * we're ok with the buffer realloc
			 */
			stream.clear();
			stream.str("");
			r->bar_ptr->pdouble += 3.7;
			stream << r->bar_ptr->pdouble;
			stream >> r->foo_ptr->pint;
			UT_ASSERTeq(r->foo_ptr->pint, 16);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	cleanup_foobar(pop);
}

/*
 * swap_test -- (internal) perform basic swap tests on p<>
 */
void
swap_test(nvobj::pool_base &pop)
{
	struct _bar {
		nvobj::p<int> value;
	};

	nvobj::persistent_ptr<_bar> swap_one;
	nvobj::persistent_ptr<_bar> swap_two;
	try {
		nvobj::transaction::run(pop, [&] {
			swap_one = pmemobj_tx_zalloc(sizeof(_bar), 0);
			swap_two = pmemobj_tx_zalloc(sizeof(_bar), 0);
		});

		nvobj::transaction::run(pop, [&] {
			swap_one->value = 1;
			swap_two->value = 2;

			swap(swap_one->value, swap_two->value);
			UT_ASSERTeq(swap_one->value, 2);
			UT_ASSERTeq(swap_two->value, 1);

			swap(swap_two->value, swap_one->value);
			UT_ASSERTeq(swap_one->value, 1);
			UT_ASSERTeq(swap_two->value, 2);

			pmemobj_tx_free(swap_one.raw());
			pmemobj_tx_free(swap_two.raw());
		});
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
		pop = nvobj::pool<struct root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	arithmetic_test(pop);
	bitwise_test(pop);
	stream_test(pop);
	swap_test(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
