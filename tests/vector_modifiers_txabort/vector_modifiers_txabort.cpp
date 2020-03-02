// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "helper_classes.hpp"
#include "list_wrapper.hpp"

#include <libpmemobj++/make_persistent.hpp>

#include <vector>

namespace nvobj = pmem::obj;

using C = container_t<int>;
using C2 = container_t<move_only>;

struct root {
	nvobj::persistent_ptr<C> v1;
	nvobj::persistent_ptr<C> v2;
	nvobj::persistent_ptr<C2> v3;
};

using It = C::const_iterator;

void
check_value(It start, It end, int value)
{
	for (auto it = start; it != end; ++it) {
		UT_ASSERTeq(*it, value);
	}
}

void
check_vector(nvobj::persistent_ptr<C> pptr, size_t count, int value)
{
	UT_ASSERTeq(pptr->size(), count);

	check_value(pptr->cbegin(), pptr->cend(), value);
}

void
check_range(It start, It end, int value)
{
	check_value(start, end, value);
}

/**
 * Test pmem::obj::vector modifiers
 *
 * Checks if vector's state is reverted when transaction aborts.
 * Methods under test:
 * - clear()
 * - resize()
 * - resize() with value
 * - swap()
 * - insert() single element version
 * - insert() fill version
 * - insert() range version
 * - insert() move version
 * - insert() initializer list version
 * - erase() single element version
 * - erase() range version
 * - pop_back()
 * - push_back() copy version
 * - push_back() move version
 * - emplace()
 * - emplace_back()
 */
void
test(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	check_vector(r->v1, 100, 1);

	bool exception_thrown = false;

	/* test clear() revert */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->clear();
			UT_ASSERT(r->v1->empty());
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test resize() revert */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->resize(50);
			UT_ASSERT(r->v1->size() == 50);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test resize() overload with value revert */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->resize(150, 2);
			UT_ASSERT(r->v1->size() == 150);
			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test swap() */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->swap(*r->v2);

			check_vector(r->v1, 50, 2);
			check_vector(r->v2, 100, 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);
	check_vector(r->v2, 50, 2);

	UT_ASSERT(exception_thrown);

	/* test insert() single element version */
	try {
		auto pos = r->v1->begin() + 50;
		nvobj::transaction::run(pop, [&] {
			r->v1->insert(pos, 5);

			/* pos has invalided after reallocation */
			auto pos_new = r->v1->begin() + 50;

			UT_ASSERT(r->v1->size() == 101);
			check_range(r->v1->begin(), pos_new, 1);
			check_range(pos_new, pos_new + 1, 5);
			check_range(pos_new + 1, r->v1->end(), 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test insert() fill version */
	try {
		auto pos = r->v1->begin() + 50;
		nvobj::transaction::run(pop, [&] {
			r->v1->insert(pos, 10, 5);

			/* pos has invalided after reallocation */
			auto pos_new = r->v1->begin() + 50;

			UT_ASSERT(r->v1->size() == 110);
			check_range(r->v1->begin(), pos_new, 1);
			check_range(pos_new, pos_new + 10, 5);
			check_range(pos_new + 10, r->v1->end(), 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test insert() range version */
	try {
		std::vector<int> v(10, 5);
		auto pos = r->v1->begin() + 50;
		nvobj::transaction::run(pop, [&] {
			r->v1->insert(pos, v.begin(), v.end());

			/* pos has invalided after reallocation */
			auto pos_new = r->v1->begin() + 50;

			UT_ASSERT(r->v1->size() == 110);
			check_range(r->v1->begin(), pos_new, 1);
			check_range(pos_new, pos_new + 10, 5);
			check_range(pos_new + 10, r->v1->end(), 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test insert() move version */
	try {
		int a = 5;
		auto pos = r->v1->begin() + 50;
		nvobj::transaction::run(pop, [&] {
			r->v1->insert(pos, std::move(a));

			/* pos has invalided after reallocation */
			auto pos_new = r->v1->begin() + 50;

			UT_ASSERT(r->v1->size() == 101);
			check_range(r->v1->begin(), pos_new, 1);
			check_range(pos_new, pos_new + 1, 5);
			check_range(pos_new + 1, r->v1->end(), 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test insert() initializer list version */
	try {
		auto pos = r->v1->begin() + 50;
		nvobj::transaction::run(pop, [&] {
			r->v1->insert(pos, {5, 5, 5, 5, 5});

			/* pos has invalided after reallocation */
			auto pos_new = r->v1->begin() + 50;

			UT_ASSERT(r->v1->size() == 105);
			check_range(r->v1->begin(), pos_new, 1);
			check_range(pos_new, pos_new + 5, 5);
			check_range(pos_new + 5, r->v1->end(), 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test erase() single element version */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->erase(r->v1->begin());

			check_vector(r->v1, 99, 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test erase() range version */
	try {
		nvobj::transaction::run(pop, [&] {
			auto pos = r->v1->begin();
			r->v1->erase(pos, pos + 10);

			check_vector(r->v1, 90, 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test erase() range version at end */
	try {
		nvobj::transaction::run(pop, [&] {
			auto pos = r->v1->begin();
			r->v1->erase(pos + 90, r->v1->end());

			check_vector(r->v1, 90, 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test pop_back() */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->pop_back();

			check_vector(r->v1, 99, 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test push_back() copy version */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->push_back(1);

			check_vector(r->v1, 101, 1);

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	check_vector(r->v1, 100, 1);

	UT_ASSERT(exception_thrown);

	/* test push_back() move version */
	UT_ASSERT(r->v3->size() == 100);

	try {
		nvobj::transaction::run(pop, [&] {
			r->v3->push_back(move_only(1));

			UT_ASSERT(r->v3->size() == 101);
			for (auto it = r->v3->cbegin(); it != r->v3->cend();
			     ++it) {
				UT_ASSERT((*it).value == 1);
			}

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v3->size() == 100);
	for (auto it = r->v3->cbegin(); it != r->v3->cend(); ++it) {
		UT_ASSERT((*it).value == 1);
	}

	UT_ASSERT(exception_thrown);

	/* test emplace() */
	UT_ASSERT(r->v1->size() == 100);

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->emplace(r->v1->begin(), 1);

			UT_ASSERT(r->v1->size() == 101);
			for (auto it = r->v1->cbegin(); it != r->v1->cend();
			     ++it) {
				UT_ASSERT(*it == 1);
			}

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v1->size() == 100);
	for (auto it = r->v1->cbegin(); it != r->v1->cend(); ++it) {
		UT_ASSERT(*it == 1);
	}

	UT_ASSERT(exception_thrown);

	/* test emplace_back() */
	try {
		nvobj::transaction::run(pop, [&] {
			r->v1->emplace_back(1);

			UT_ASSERT(r->v1->size() == 101);
			for (auto it = r->v1->cbegin(); it != r->v1->cend();
			     ++it) {
				UT_ASSERT(*it == 1);
			}

			nvobj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERT(r->v1->size() == 100);
	for (auto it = r->v1->cbegin(); it != r->v1->cend(); ++it) {
		UT_ASSERT(*it == 1);
	}

	UT_ASSERT(exception_thrown);
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest: modifiers_txabort", PMEMOBJ_MIN_POOL * 2,
		S_IWUSR | S_IRUSR);

	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v1 = nvobj::make_persistent<C>(100U, 1);
			r->v2 = nvobj::make_persistent<C>(50U, 2);
			r->v3 = nvobj::make_persistent<C2>(100U);
		});

		test(pop);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<C>(r->v1);
			nvobj::delete_persistent<C>(r->v2);
			nvobj::delete_persistent<C2>(r->v3);
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
