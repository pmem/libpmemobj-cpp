// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "radix.hpp"

#include <random>
#include <unordered_map>
#include <unordered_set>

void
test_iterators(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_int = nvobj::make_persistent<container_int>();
		r->radix_int->try_emplace("", 0U);
		r->radix_int->try_emplace("ab", 1U);
		r->radix_int->try_emplace("ba", 2U);
		r->radix_int->try_emplace("a", 3U);
		r->radix_int->try_emplace("b", 4U);

		r->radix_str = nvobj::make_persistent<container_string>();
		r->radix_str->try_emplace("", "");
		r->radix_str->try_emplace(" ", "ab");
		r->radix_str->try_emplace("  ", "ab");

		r->radix_str->try_emplace("ab", "ab");
		r->radix_str->try_emplace("ba", "ba");
		r->radix_str->try_emplace("a", "a");
		r->radix_str->try_emplace("b", "b");
	});

	auto it = r->radix_int->find("a");
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

	it = r->radix_int->erase(it);
	UT_ASSERT(it.key().compare(std::string("a")) == 0);
	UT_ASSERTeq(it.value(), 3);

	(*it).second = 4;
	UT_ASSERT(it.key().compare(std::string("a")) == 0);
	UT_ASSERTeq(it.value(), 4);

	it = r->radix_int->lower_bound("b");
	UT_ASSERT(it.key().compare(std::string("b")) == 0);

	it = r->radix_int->lower_bound("aa");
	UT_ASSERT(it.key().compare(std::string("ab")) == 0);

	auto it2 = r->radix_str->lower_bound("aa");
	it2.value() = "xx";

	auto long_string = std::string(1024, 'x');
	it2.value() = long_string;

	UT_ASSERT(r->radix_str->find("") != r->radix_str->end());
	UT_ASSERT(r->radix_str->find(" ") != r->radix_str->end());
	UT_ASSERT(r->radix_str->find(" ") != r->radix_str->end());

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
		nvobj::delete_persistent<container_int>(r->radix_int);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

void
test_ref_stability(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_str = nvobj::make_persistent<container_string>();
		r->radix_int = nvobj::make_persistent<container_int>();
	});

	{
		using ref_type = typename container_string::reference;

		ref_type ab_ref = *r->radix_str->emplace("ab", "ab").first;
		ref_type a_ref = *r->radix_str->emplace("a", "a").first;
		ref_type acxxxy_ref =
			*r->radix_str->emplace("acxxxy", "acxxxy").first;
		ref_type acxxxz_ref =
			*r->radix_str->emplace("acxxxz", "acxxxz").first;

		UT_ASSERT(nvobj::string_view(ab_ref.second).compare("ab") == 0);
		UT_ASSERT(nvobj::string_view(a_ref.second).compare("a") == 0);
		UT_ASSERT(nvobj::string_view(acxxxy_ref.second)
				  .compare("acxxxy") == 0);
		UT_ASSERT(nvobj::string_view(acxxxz_ref.second)
				  .compare("acxxxz") == 0);
	}

	{
		using ref_type = typename container_int::reference;

		ref_type ab_ref = *r->radix_int->emplace("ab", 1U).first;
		ref_type a_ref = *r->radix_int->emplace("a", 2U).first;
		ref_type acxxxy_ref =
			*r->radix_int->emplace("acxxxy", 3U).first;
		ref_type acxxxz_ref =
			*r->radix_int->emplace("acxxxz", 4U).first;

		UT_ASSERT(ab_ref.second == 1U);
		UT_ASSERT(a_ref.second == 2U);
		UT_ASSERT(acxxxy_ref.second == 3U);
		UT_ASSERT(acxxxz_ref.second == 4U);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
		nvobj::delete_persistent<container_int>(r->radix_int);
	});
}

/* Tests some corner cases (not covered by libcxx find tests). */
void
test_find(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_str = nvobj::make_persistent<container_string>();
	});

	r->radix_str->emplace("ab", "");
	r->radix_str->emplace("a", "");
	r->radix_str->emplace("acxxxy", "");
	r->radix_str->emplace("acxxxz", "");

	/* Assert no such elements. */
	UT_ASSERT(r->radix_str->find("acxxx") == r->radix_str->end());
	UT_ASSERT(r->radix_str->find("ac") == r->radix_str->end());
	UT_ASSERT(r->radix_str->find("acyyy") == r->radix_str->end());
	UT_ASSERT(r->radix_str->find("ay") == r->radix_str->end());

	/* Find will descend to the acxxxy and fail only after comparing keys.
	 */
	UT_ASSERT(r->radix_str->find("acyyyy") == r->radix_str->end());
	UT_ASSERT(r->radix_str->find("acyyyz") == r->radix_str->end());

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

/* Tests some corner cases (not covered by libcxx erase tests). */
void
test_erase(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_str = nvobj::make_persistent<container_string>();
	});

	std::unordered_set<std::string> set = {"ab", "a", "acxxxx", "acxxxy",
					       "acxxxz"};

	/* Used for testing iterator stability. */
	std::unordered_map<std::string, typename container_string::iterator>
		its;

	for (auto &s : set) {
		auto ret = r->radix_str->emplace(s, s);
		UT_ASSERT(ret.second);
		its.emplace(s, ret.first);
	}

	auto erase = [&](std::string key) {
		UT_ASSERT(r->radix_str->erase(key) == 1);
		set.erase(key);

		for (auto &s : set) {
			auto it = r->radix_str->find(s);
			UT_ASSERT(it != r->radix_str->end());
			UT_ASSERT(nvobj::string_view(it->second).compare(s) ==
				  0);

			auto &m_it = its.find(s)->second;
			UT_ASSERT(m_it->first.compare(s) == 0);
			UT_ASSERT(nvobj::string_view(m_it->second).compare(s) ==
				  0);
		}
	};

	/* Force internal node removal. */
	erase("acxxxy");
	erase("acxxxz");

	erase("acxxxx");
	erase("ab");
	erase("a");

	UT_ASSERT(r->radix_str->size() == 0);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

void
test_binary_keys(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_int_int = nvobj::make_persistent<container_int_int>();
	});

	/* Used for testing iterator stability. */
	std::unordered_map<unsigned, typename container_int_int::iterator> its;

	for (unsigned i = std::numeric_limits<uint16_t>::max(); i > 0; i--) {
		auto ret = r->radix_int_int->emplace(i - 1, i - 1);
		UT_ASSERT(ret.second);
		its.emplace(i - 1, ret.first);
	}

	verify_elements(r->radix_int_int, std::numeric_limits<uint16_t>::max(),
			[](unsigned i) { return i; });

	/* Used for testing iterator stability. In each iteration one element
	 * is erased. This erasure should not affect further checks. */
	for (unsigned i = std::numeric_limits<uint16_t>::max(); i > 0; i--) {
		auto &it = its.find(i - 1)->second;
		UT_ASSERT(it->first == i - 1);
		UT_ASSERT(it->second == i - 1);

		r->radix_int_int->erase(i - 1);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_int_int>(r->radix_int_int);
	});

	its = {};

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));

	nvobj::transaction::run(pop, [&] {
		r->radix_int_int = nvobj::make_persistent<container_int_int>();
	});

	for (unsigned i = 0; i < std::numeric_limits<uint16_t>::max(); i++) {
		auto ret = r->radix_int_int->emplace(i, i);
		UT_ASSERT(ret.second);
		its.emplace(i, ret.first);
	}

	verify_elements(r->radix_int_int, std::numeric_limits<uint16_t>::max(),
			[](unsigned i) { return i; });

	/* Used for testing iterator stability. In each iteration one element
	 * is erased. This erasure should not affect further checks. */
	for (unsigned i = 0; i < std::numeric_limits<uint16_t>::max(); i++) {
		auto &it = its.find(i)->second;
		UT_ASSERT(it->first == i);
		UT_ASSERT(it->second == i);

		r->radix_int_int->erase(i);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_int_int>(r->radix_int_int);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

void
test_pre_post_fixes(nvobj::pool<root> &pop)
{
	std::random_device rd;
	auto seed = rd();
	std::cout << "rand seed: " << seed << std::endl;
	auto generator = std::mt19937_64(seed);

	auto num_elements = (1U << 10);

	std::vector<std::string> elements;
	elements.reserve(num_elements);

	elements.push_back("0");

	/* Used for testing iterator stability. */
	std::unordered_map<std::string, typename container_string::iterator>
		its;

	/* This loop creates string so that elements[i] is prefix of
	 * elements[i + 1] and they differ only by 4 bits:
	 * '0xA0', '0xA0 0xAB', '0xA0 0xAB 0xC0', '0xA0 0xAB 0xC0 0xCD'
	 */
	for (unsigned i = 1; i < num_elements * 2; i++) {
		if (i % 2 == 0)
			elements.push_back(
				elements.back() +
				std::string(1, char(generator() % 128)));
		else {
			auto str = elements.back();
			str.back() |= (-(char(generator() % 128)));
			elements.push_back(str);
		}
	}

	std::vector<std::string> s_elements = elements;
	std::sort(s_elements.begin(), s_elements.end());

	/* there might be some duplicates so update the total size */
	s_elements.erase(std::unique(s_elements.begin(), s_elements.end()),
			 s_elements.end());
	num_elements = s_elements.size();

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_str = nvobj::make_persistent<container_string>();
	});

	for (unsigned i = elements.size(); i > 0; i--) {
		auto ret =
			r->radix_str->emplace(elements[i - 1], elements[i - 1]);
		if (ret.second)
			its.emplace(elements[i - 1], ret.first);
	}

	UT_ASSERTeq(r->radix_str->size(), num_elements);
	unsigned i = 0;
	for (auto it = r->radix_str->begin(); it != r->radix_str->end();
	     ++it, ++i) {
		UT_ASSERT(it.key() == s_elements[i]);
	}

	/* Used for testing iterator stability. */
	for (unsigned i = num_elements; i > 0; i--) {
		auto &it = its.find(s_elements[i - 1])->second;
		UT_ASSERT(it->first.compare(s_elements[i - 1]) == 0);
		UT_ASSERT(nvobj::string_view(it->second)
				  .compare(s_elements[i - 1]) == 0);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
	});

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
		pop = nvobj::pool<struct root>::create(path, "radix_basic",
						       10 * PMEMOBJ_MIN_POOL,
						       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_ref_stability(pop);
	test_iterators(pop);
	test_find(pop);
	test_erase(pop);
	test_binary_keys(pop);
	test_pre_post_fixes(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
