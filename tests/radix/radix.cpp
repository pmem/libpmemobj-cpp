// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/experimental/radix.hpp>

#include <libpmemobj++/inline_string.hpp>

namespace nvobj = pmem::obj;

using container = nvobj::radix_tree<int>;
using container_in = nvobj::radix_tree<pmem::obj::inline_string>;

struct root {
	nvobj::persistent_ptr<container> radix;
	nvobj::persistent_ptr<container_in> radix_in;
};

namespace
{
void
test_emplace(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix = nvobj::make_persistent<container>();
		r->radix->emplace("", 0);
		r->radix->emplace("ab", 1);
		r->radix->emplace("ba", 2);
		r->radix->emplace("a", 3);
		r->radix->emplace("b", 4);

		r->radix_in = nvobj::make_persistent<container_in>();
		r->radix_in->emplace("", "");
		r->radix_in->emplace(" ", "ab");
		r->radix_in->emplace("  ", "ab");

		r->radix_in->emplace("ab", "ab");
		r->radix_in->emplace("ba", "ba");
		r->radix_in->emplace("a", "a");
		r->radix_in->emplace("b", "b");
	});

	auto it = r->radix->find("a");
	UT_ASSERT(it.key().compare(std::string("a")) == 0);
	UT_ASSERTeq(it.value(), 3);

	++it;
	UT_ASSERT(it.key().compare(std::string("ab")) == 0);
	UT_ASSERTeq(it.value(), 1);

	++it;
	UT_ASSERT(it.key().compare(std::string("b")) == 0);
	UT_ASSERTeq(it.value(), 4);

	++it;
	UT_ASSERT(it.key().compare(std::string("ba")) == 0);
	UT_ASSERTeq(it.value(), 2);

	--it;
	UT_ASSERT(it.key().compare(std::string("b")) == 0);
	UT_ASSERTeq(it.value(), 4);

	--it;
	UT_ASSERT(it.key().compare(std::string("ab")) == 0);
	UT_ASSERTeq(it.value(), 1);

	--it;
	UT_ASSERT(it.key().compare(std::string("a")) == 0);
	UT_ASSERTeq(it.value(), 3);

	--it;
	UT_ASSERT(it.key().compare(std::string("")) == 0);
	UT_ASSERTeq(it.value(), 0);

	it = r->radix->erase(it);
	UT_ASSERT(it.key().compare(std::string("a")) == 0);
	UT_ASSERTeq(it.value(), 3);

	(*it).second = 4;
	UT_ASSERT(it.key().compare(std::string("a")) == 0);
	UT_ASSERTeq(it.value(), 4);

	it = r->radix->lower_bound("b");
	UT_ASSERT(it.key().compare(std::string("b")) == 0);

	it = r->radix->lower_bound("aa");
	UT_ASSERT(it.key().compare(std::string("ab")) == 0);

	auto it2 = r->radix_in->lower_bound("aa");
	it2.assign("xx");

	auto long_string = std::string(1024, 'x');
	it2.assign(long_string);

	UT_ASSERT(r->radix_in->find("") != r->radix_in->end());
	UT_ASSERT(r->radix_in->find(" ") != r->radix_in->end());
	UT_ASSERT(r->radix_in->find(" ") != r->radix_in->end());
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
			path, "XX", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_emplace(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
