// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "radix.hpp"

static const unsigned N_ELEMS = 300;

template <typename Container>
std::vector<typename Container::iterator>
init(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	std::vector<typename Container::iterator> its;

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		for (unsigned i = 0; i < N_ELEMS; i++) {
			auto ret = ptr->try_emplace(key<Container>(i),
						    value<Container>(i));
			its.push_back(ret.first);

			ptr->runtime_initialize_mt();
		}
	});

	return its;
}

template <typename Container>
void
test_memory_reclamation_erase(nvobj::pool<root> &pop,
			      nvobj::persistent_ptr<Container> &ptr)
{
	auto its = init(pop, ptr);

	for (auto it = ptr->begin(); it != ptr->end();)
		it = ptr->erase(it);

	for (unsigned i = 0; i < its.size(); i++) {
		UT_ASSERT(its[i]->key() == key<Container>(i));
		UT_ASSERT(its[i]->value() == value<Container>(i));
	}

	UT_ASSERT(num_allocs(pop) > 2);

	ptr->garbage_collect();

	/* radix_tree and garbage vector */
	UT_ASSERTeq(num_allocs(pop), 2);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

template <typename Container>
void
test_memory_reclamation_assign(nvobj::pool<root> &pop,
			       nvobj::persistent_ptr<Container> &ptr)
{
	auto its = init(pop, ptr);

	for (unsigned i = 0; i < its.size(); i++) {
		auto ret = ptr->insert_or_assign(key<Container>(i),
						 value<Container>(1 + i));
		UT_ASSERT(!ret.second);
	}

	for (unsigned i = 0; i < its.size(); i++) {
		UT_ASSERT(its[i]->key() == key<Container>(i));
		UT_ASSERT(its[i]->value() == value<Container>(i));
	}

	auto alocs = num_allocs(pop);

	ptr->garbage_collect();

	UT_ASSERT(num_allocs(pop) < alocs);

	for (unsigned i = 0; i < its.size(); i++) {
		auto it = ptr->find(key<Container>(i));

		UT_ASSERT(it->key() == key<Container>(i));
		UT_ASSERT(it->value() == value<Container>(i + 1));
	}

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

template <typename Container>
void
test_memory_reclamation_dtor(nvobj::pool<root> &pop,
			     nvobj::persistent_ptr<Container> &ptr)
{
	init(pop, ptr);

	for (auto it = ptr->begin(); it != ptr->end();)
		it = ptr->erase(it);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERTeq(num_allocs(pop), 0);
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<struct root>::create(path, "radix_basic",
						       10 * PMEMOBJ_MIN_POOL,
						       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_memory_reclamation_erase(pop, pop.root()->radix_str);
	test_memory_reclamation_erase(pop, pop.root()->radix_int_int);

	test_memory_reclamation_assign(pop, pop.root()->radix_str);
	test_memory_reclamation_assign(pop, pop.root()->radix_int_int);

	test_memory_reclamation_dtor(pop, pop.root()->radix_str);
	test_memory_reclamation_dtor(pop, pop.root()->radix_int_int);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
