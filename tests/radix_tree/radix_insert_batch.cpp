// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#include "radix.hpp"

template <typename Container>
void
test_insert_batch(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<Container>(); });

	container_int_int::node_handle_batch b1;
	for (size_t i = 0; i < 1000; ++i)
		ptr->create_node(b1, i, i);
	ptr->insert(b1);

	for (size_t i = 0; i < 1000; ++i) {
		auto res = ptr->find(i);
		UT_ASSERTeq(res->key(), i);
		UT_ASSERTeq(res->value(), i);
	}

	container_int_int::node_handle_batch b2;
	for (size_t i = 0; i < 1000; ++i)
		ptr->create_node(b2, i, i + 1);
	ptr->insert(b2);

	for (size_t i = 0; i < 1000; ++i) {
		auto res = ptr->find(i);
		UT_ASSERTeq(res->key(), i);
		UT_ASSERTeq(res->value(), i);
	}

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<struct root>::create(path, "radix",
						       10 * PMEMOBJ_MIN_POOL,
						       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_insert_batch<container_int_int>(pop, pop.root()->radix_int_int);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
