// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "radix.hpp"

#include <algorithm>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include <sstream>

static std::mt19937_64 generator;

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
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("a")) == 0);
	UT_ASSERTeq(it->value(), 3);

	++it;
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("ab")) ==
		  0);
	UT_ASSERTeq(it->value(), 1);

	++it;
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("b")) == 0);
	UT_ASSERTeq(it->value(), 4);

	++it;
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("ba")) ==
		  0);
	UT_ASSERTeq(it->value(), 2);

	--it;
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("b")) == 0);
	UT_ASSERTeq(it->value(), 4);

	--it;
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("ab")) ==
		  0);
	UT_ASSERTeq(it->value(), 1);

	--it;
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("a")) == 0);
	UT_ASSERTeq(it->value(), 3);

	--it;
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("")) == 0);
	UT_ASSERTeq(it->value(), 0);

	it = r->radix_int->erase(it);
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("a")) == 0);
	UT_ASSERTeq(it->value(), 3);

	(*it).value() = 4;
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("a")) == 0);
	UT_ASSERTeq(it->value(), 4);

	it = r->radix_int->lower_bound("b");
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("b")) == 0);

	it = r->radix_int->lower_bound("aa");
	UT_ASSERT(nvobj::string_view(it->key()).compare(std::string("ab")) ==
		  0);

	auto it2 = r->radix_str->find("");

	it2 = r->radix_str->lower_bound("aa");
	auto it3 = it2;
	it2.assign_val("xx");

	UT_ASSERT(nvobj::string_view(it2->value()).compare("xx") == 0);
	UT_ASSERT(nvobj::string_view(it3->value()).compare("xx") == 0);

	auto long_string = std::string(1024, 'x');
	/* The previous assignment should not invalidate the iterator. */
	it2.assign_val(long_string);

	UT_ASSERT(nvobj::string_view(it2->value()).compare(long_string) == 0);

	UT_ASSERT(nvobj::string_view(r->radix_str->lower_bound("aa")->value())
			  .compare(long_string) == 0);

	UT_ASSERT(r->radix_str->find("") != r->radix_str->end());
	UT_ASSERT(r->radix_str->find(" ") != r->radix_str->end());
	UT_ASSERT(r->radix_str->find(" ") != r->radix_str->end());

	/* Verify operator<< compiles. */
	std::stringstream ss;
	ss << *r->radix_str;

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
		auto &ab_ref = *r->radix_str->emplace("ab", "ab").first;
		auto &a_ref = *r->radix_str->emplace("a", "a").first;
		auto &acxxxy_ref =
			*r->radix_str->emplace("acxxxy", "acxxxy").first;
		auto &acxxxz_ref =
			*r->radix_str->emplace("acxxxz", "acxxxz").first;

		UT_ASSERT(nvobj::string_view(ab_ref.value()).compare("ab") ==
			  0);
		UT_ASSERT(nvobj::string_view(a_ref.value()).compare("a") == 0);
		UT_ASSERT(nvobj::string_view(acxxxy_ref.value())
				  .compare("acxxxy") == 0);
		UT_ASSERT(nvobj::string_view(acxxxz_ref.value())
				  .compare("acxxxz") == 0);
	}

	{
		auto &ab_ref = *r->radix_int->emplace("ab", 1U).first;
		auto &a_ref = *r->radix_int->emplace("a", 2U).first;
		auto &acxxxy_ref = *r->radix_int->emplace("acxxxy", 3U).first;
		auto &acxxxz_ref = *r->radix_int->emplace("acxxxz", 4U).first;

		UT_ASSERT(ab_ref.value() == 1U);
		UT_ASSERT(a_ref.value() == 2U);
		UT_ASSERT(acxxxy_ref.value() == 3U);
		UT_ASSERT(acxxxz_ref.value() == 4U);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
		nvobj::delete_persistent<container_int>(r->radix_int);
	});
}

/* Tests some corner cases (not covered by libcxx find/bound tests). */
void
test_find(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	{
		nvobj::transaction::run(pop, [&] {
			r->radix_str =
				nvobj::make_persistent<container_string>();
		});

		UT_ASSERT(r->radix_str->lower_bound("") == r->radix_str->end());
		UT_ASSERT(r->radix_str->upper_bound("") == r->radix_str->end());

		r->radix_str->emplace("ab", "");
		r->radix_str->emplace("a", "");
		r->radix_str->emplace("acxxxy", "");
		r->radix_str->emplace("acxxxz", "");

		UT_ASSERT(r->radix_str->lower_bound("acxxxyy") ==
			  r->radix_str->find("acxxxz"));
		UT_ASSERT(r->radix_str->upper_bound("acxxxyy") ==
			  r->radix_str->find("acxxxz"));

		/* Assert no such elements. */
		UT_ASSERT(r->radix_str->find("acxxx") == r->radix_str->end());
		UT_ASSERT(r->radix_str->find("ac") == r->radix_str->end());
		UT_ASSERT(r->radix_str->find("acyyy") == r->radix_str->end());
		UT_ASSERT(r->radix_str->find("ay") == r->radix_str->end());
		UT_ASSERT(r->radix_str->lower_bound("acxxxzz") ==
			  r->radix_str->end());
		UT_ASSERT(r->radix_str->upper_bound("acxxxzz") ==
			  r->radix_str->end());

		/* Find will descend to the acxxxy and fail only after comparing
		 * keys.
		 */
		UT_ASSERT(r->radix_str->find("acyyyy") == r->radix_str->end());
		UT_ASSERT(r->radix_str->find("acyyyz") == r->radix_str->end());

		/* Test *_bound when leaf nodes' keys differ from the
		 * searched-for key at compressed bytes. */
		UT_ASSERT(r->radix_str->lower_bound("acaaay") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->upper_bound("acaaay") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->lower_bound("acyyyy") ==
			  r->radix_str->end());
		UT_ASSERT(r->radix_str->upper_bound("acyyyy") ==
			  r->radix_str->end());
		UT_ASSERT(r->radix_str->lower_bound("acyyy") ==
			  r->radix_str->end());
		UT_ASSERT(r->radix_str->upper_bound("acyyy") ==
			  r->radix_str->end());

		/* Look for key which shares some common part with leafs but
		 * differs on compressed bytes. */
		UT_ASSERT(r->radix_str->lower_bound("acx") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->upper_bound("acx") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->lower_bound("acxxx") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->upper_bound("acxxx") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->lower_bound("acxxa") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->upper_bound("acxxa") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->lower_bound("acaaa") ==
			  r->radix_str->find("acxxxy"));
		UT_ASSERT(r->radix_str->upper_bound("acaaa") ==
			  r->radix_str->find("acxxxy"));

		r->radix_str->emplace("ad", "");

		UT_ASSERT(r->radix_str->lower_bound("acyyyy") ==
			  r->radix_str->find("ad"));
		UT_ASSERT(r->radix_str->upper_bound("acyyyy") ==
			  r->radix_str->find("ad"));
		UT_ASSERT(r->radix_str->lower_bound("acyyy") ==
			  r->radix_str->find("ad"));
		UT_ASSERT(r->radix_str->upper_bound("acyyy") ==
			  r->radix_str->find("ad"));
		UT_ASSERT(r->radix_str->lower_bound("acxxxzz") ==
			  r->radix_str->find("ad"));
		UT_ASSERT(r->radix_str->upper_bound("acxxxzz") ==
			  r->radix_str->find("ad"));

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_string>(
				r->radix_str);
		});

		UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
	}

	{
		nvobj::transaction::run(pop, [&] {
			r->radix_str =
				nvobj::make_persistent<container_string>();
		});

		r->radix_str->emplace("a", "");
		r->radix_str->emplace("bccc", "");
		r->radix_str->emplace("bccca", "");
		r->radix_str->emplace("bcccbccc", "");

		UT_ASSERT(r->radix_str->lower_bound("baaaba") ==
			  r->radix_str->find("bccc"));
		UT_ASSERT(r->radix_str->upper_bound("baaaba") ==
			  r->radix_str->find("bccc"));
		UT_ASSERT(r->radix_str->lower_bound("ba") ==
			  r->radix_str->find("bccc"));
		UT_ASSERT(r->radix_str->upper_bound("ba") ==
			  r->radix_str->find("bccc"));
		UT_ASSERT(r->radix_str->lower_bound("b") ==
			  r->radix_str->find("bccc"));
		UT_ASSERT(r->radix_str->upper_bound("b") ==
			  r->radix_str->find("bccc"));
		UT_ASSERT(r->radix_str->lower_bound("bddd") ==
			  r->radix_str->end());
		UT_ASSERT(r->radix_str->upper_bound("bddd") ==
			  r->radix_str->end());

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_string>(
				r->radix_str);
		});

		UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
	}

	{
		auto r = pop.root();

		nvobj::transaction::run(pop, [&] {
			r->radix_str =
				nvobj::make_persistent<container_string>();
		});

		r->radix_str->try_emplace("Y", "1");
		r->radix_str->try_emplace("YB", "2");
		r->radix_str->try_emplace("YC", "3");
		r->radix_str->try_emplace("Z", "4");
		r->radix_str->try_emplace("ZB", "5");
		r->radix_str->try_emplace("ZC", "6");
		r->radix_str->try_emplace("ZD", "7");

		auto lb = r->radix_str->lower_bound("");
		auto ub = r->radix_str->upper_bound("@@@@");

		UT_ASSERT(std::distance(lb, ub) == 0);

		lb = r->radix_str->lower_bound("");
		ub = r->radix_str->upper_bound("ZZZZ");

		UT_ASSERT(std::distance(lb, ub) == 7);

		lb = r->radix_str->lower_bound("");
		ub = r->radix_str->upper_bound("ZZZZ");

		UT_ASSERT(std::distance(lb, ub) == 7);
		r->radix_str->clear();

		r->radix_str->try_emplace("A", "1");
		r->radix_str->try_emplace("AB", "2");
		r->radix_str->try_emplace("AC", "3");
		r->radix_str->try_emplace("B", "4");
		r->radix_str->try_emplace("BB", "5");
		r->radix_str->try_emplace("BC", "6");
		r->radix_str->try_emplace("BD", "7");

		lb = r->radix_str->lower_bound("");
		ub = r->radix_str->upper_bound("CCCC");

		UT_ASSERT(std::distance(lb, ub) == 7);

		lb = r->radix_str->lower_bound("");
		ub = r->radix_str->upper_bound("ZZZZ");

		UT_ASSERT(std::distance(lb, ub) == 7);
		r->radix_str->clear();

		r->radix_str->try_emplace("A", "1");
		r->radix_str->try_emplace("AB", "2");
		r->radix_str->try_emplace("AC", "3");
		r->radix_str->try_emplace("C", "4");
		r->radix_str->try_emplace("CB", "5");
		r->radix_str->try_emplace("CC", "6");
		r->radix_str->try_emplace("CD", "7");

		lb = r->radix_str->lower_bound("");
		ub = r->radix_str->upper_bound("BB");

		UT_ASSERT(std::distance(lb, ub) == 3);
		r->radix_str->clear();

		r->radix_str->try_emplace("A", "1");
		r->radix_str->try_emplace("AB", "2");
		r->radix_str->try_emplace("AC", "3");
		r->radix_str->try_emplace("Y", "4");
		r->radix_str->try_emplace("YB", "5");
		r->radix_str->try_emplace("YC", "6");
		r->radix_str->try_emplace("YD", "7");

		lb = r->radix_str->lower_bound("");
		ub = r->radix_str->upper_bound("@@@@");

		UT_ASSERT(std::distance(lb, ub) == 0);

		char ch[] = {char((15 << 4)), 0};
		r->radix_str->try_emplace(ch, "8");

		lb = r->radix_str->lower_bound("");
		ub = r->radix_str->upper_bound("@@@@");

		UT_ASSERT(std::distance(lb, ub) == 0);

		char last_slot[] = {char((15 << 4) | 15), 0};
		lb = r->radix_str->lower_bound("");
		ub = r->radix_str->upper_bound(last_slot);

		UT_ASSERTeq(std::distance(lb, ub), 8);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<container_string>(
				r->radix_str);
		});

		UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
	}
}

const auto compressed_path_len = 4;
const auto num_children = 3;

static void
generate_compressed_tree(nvobj::persistent_ptr<container_string> ptr,
			 std::string prefix, int level)
{
	/* it should be just one digit */
	UT_ASSERT(num_children <= 9);

	if (!level)
		return;

	/* Test assumes characters are > 33 and < 122 (printable chars) */
	auto compressed_path =
		std::string(compressed_path_len, char(generator() % 87 + 34));
	for (int i = 0; i < num_children; i++) {
		auto key = prefix + std::to_string(i) + compressed_path;

		auto ret = ptr->try_emplace(key, "");
		UT_ASSERT(ret.second);

		generate_compressed_tree(ptr, key, level - 1);
	}
}

static void
verify_bounds(nvobj::persistent_ptr<container_string> ptr,
	      const std::vector<std::string> &keys)
{
	for (size_t i = 0; i < keys.size() - 1; i++) {
		/* generate key k for which k < keys[i] && k >= keys[i - 1] */
		auto k = keys[i];
		k[k.size() - 1]--;

		if (i > 0)
			UT_ASSERT(k > keys[i - 1]);

		auto it = ptr->upper_bound(k);
		UT_ASSERT(it->key() == keys[i]);

		it = ptr->lower_bound(k);
		UT_ASSERT(it->key() == keys[i]);
	}
}

static void
verify_bounds_key(nvobj::persistent_ptr<container_string> ptr,
		  const std::vector<std::string> &keys, const std::string &key)
{
	auto expected = std::lower_bound(keys.begin(), keys.end(), key);
	auto actual = ptr->lower_bound(key);
	UT_ASSERT((expected == keys.end() && actual == ptr->end()) ||
		  actual->key() == *expected);

	expected = std::upper_bound(keys.begin(), keys.end(), key);
	actual = ptr->upper_bound(key);
	UT_ASSERT((expected == keys.end() && actual == ptr->end()) ||
		  actual->key() == *expected);
}

void
test_compression(nvobj::pool<root> &pop)
{
	const auto num_levels = 3;

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_str = nvobj::make_persistent<container_string>();
	});

	generate_compressed_tree(r->radix_str, "", num_levels);

	std::vector<std::string> keys;
	for (auto &e : (*r->radix_str))
		keys.emplace_back(e.key().data(), e.key().size());

	auto test_keys = keys;
	std::sort(test_keys.begin(), test_keys.end());
	UT_ASSERT(test_keys == keys);

	verify_bounds(r->radix_str, keys);

	for (size_t i = 1; i < keys.size() - 1; i++) {
		/* Key consists of segments like this:
		 * N-path-M-path ... where N, M is child number.
		 */
		auto k = keys[i];

		k = keys[i];
		auto idx = k.size() - compressed_path_len +
			(generator() % compressed_path_len);

		/* flip some bit at the end (part of a compression) */
		k[idx] = 0;
		verify_bounds_key(r->radix_str, keys, k);
		auto lb = r->radix_str->lower_bound(k);
		auto rb = r->radix_str->upper_bound(k);
		UT_ASSERT(lb == rb);
		UT_ASSERT(r->radix_str->find(keys[i]) == lb);

		k[idx] = std::numeric_limits<char>::max();
		verify_bounds_key(r->radix_str, keys, k);

		k = keys[i];
		k[1] = 0;
		verify_bounds_key(r->radix_str, keys, k);

		k = keys[i];
		k[1] = std::numeric_limits<char>::max();
		verify_bounds_key(r->radix_str, keys, k);

		k = keys[i] + "postfix";
		verify_bounds_key(r->radix_str, keys, k);

		k = keys[i].substr(0, k.size() - compressed_path_len - 1);
		verify_bounds_key(r->radix_str, keys, k);

		k = keys[i].substr(0, k.size() - compressed_path_len);
		verify_bounds_key(r->radix_str, keys, k);

		k = keys[i].substr(0, k.size() - 1);
		verify_bounds_key(r->radix_str, keys, k);
	}

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

	std::unordered_set<std::string> set = {"b", "acxxx", "acxxxa",
					       "acxxx!"};

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
			UT_ASSERT(nvobj::string_view(it->value()).compare(s) ==
				  0);

			auto &m_it = its.find(s)->second;
			UT_ASSERT(nvobj::string_view(m_it->key()).compare(s) ==
				  0);
			UT_ASSERT(
				nvobj::string_view(m_it->value()).compare(s) ==
				0);
		}
	};

	erase("acxxxa");
	erase("acxxx!");
	erase("acxxx");
	erase("b");

	UT_ASSERT(r->radix_str->size() == 0);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

/* This test inserts elements in range [0:2:2 * numeric_limits<uint16_t>::max()]
 */
void
test_binary_keys(nvobj::pool<root> &pop)
{
	auto r = pop.root();
	auto kv_f = [](unsigned i) { return i * 2; };

	nvobj::transaction::run(pop, [&] {
		r->radix_int_int = nvobj::make_persistent<container_int_int>();
	});

	/* Used for testing iterator stability. */
	std::unordered_map<unsigned, typename container_int_int::iterator> its;

	for (unsigned i = 2 * std::numeric_limits<uint16_t>::max(); i > 0;
	     i -= 2) {
		auto ret = r->radix_int_int->emplace(i - 2, i - 2);
		UT_ASSERT(ret.second);
		its.emplace(i - 2, ret.first);
	}

	UT_ASSERT(std::distance(
			  r->radix_int_int->lower_bound(0),
			  r->radix_int_int->upper_bound(
				  std::numeric_limits<uint16_t>::max() * 3)) ==
		  std::numeric_limits<uint16_t>::max());

	verify_elements(r->radix_int_int, std::numeric_limits<uint16_t>::max(),
			kv_f, kv_f);

	for (unsigned i = 1; i < 2 * std::numeric_limits<uint16_t>::max() - 2U;
	     i += 2) {
		auto lit = r->radix_int_int->lower_bound(i);
		UT_ASSERT(lit->key() == i + 1);

		auto uit = r->radix_int_int->upper_bound(i);
		UT_ASSERT(uit->key() == i + 1);
	}

	/* Used for testing iterator stability. In each iteration one element
	 * is erased. This erasure should not affect further checks. */
	for (unsigned i = 2 * std::numeric_limits<uint16_t>::max(); i > 0;
	     i -= 2) {
		auto &it = its.find(i - 2)->second;
		UT_ASSERT(it->key() == i - 2);
		UT_ASSERT(it->value() == i - 2);

		r->radix_int_int->erase(i - 2);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_int_int>(r->radix_int_int);
	});

	its = {};

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));

	nvobj::transaction::run(pop, [&] {
		r->radix_int_int = nvobj::make_persistent<container_int_int>();
	});

	for (unsigned i = 0; i < 2 * std::numeric_limits<uint16_t>::max();
	     i += 2) {
		auto ret = r->radix_int_int->emplace(i, i);
		UT_ASSERT(ret.second);
		its.emplace(i, ret.first);
	}

	verify_elements(r->radix_int_int, std::numeric_limits<uint16_t>::max(),
			kv_f, kv_f);

	for (unsigned i = 1; i < 2 * std::numeric_limits<uint16_t>::max() - 2U;
	     i += 2) {
		auto lit = r->radix_int_int->lower_bound(i);
		UT_ASSERT(lit->key() == i + 1);

		auto uit = r->radix_int_int->upper_bound(i);
		UT_ASSERT(uit->key() == i + 1);
	}

	/* Used for testing iterator stability. In each iteration one element
	 * is erased. This erasure should not affect further checks. */
	for (unsigned i = 0; i < 2 * std::numeric_limits<uint16_t>::max();
	     i += 2) {
		auto &it = its.find(i)->second;
		UT_ASSERT(it->key() == i);
		UT_ASSERT(it->value() == i);

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
				std::string(1, char(generator() % 127 + 1)));
		else {
			auto str = elements.back();
			str.back() |= (-(char(generator() % 127 + 1)));
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

	verify_bounds(r->radix_str, s_elements);

	UT_ASSERTeq(r->radix_str->size(), num_elements);
	unsigned i = 0;
	for (auto it = r->radix_str->begin(); it != r->radix_str->end();
	     ++it, ++i) {
		UT_ASSERT(nvobj::string_view(it->key()) == s_elements[i]);
	}

	/* Used for testing iterator stability. */
	for (unsigned i = num_elements; i > 0; i--) {
		auto &it = its.find(s_elements[i - 1])->second;
		UT_ASSERT(nvobj::string_view(it->key()).compare(
				  s_elements[i - 1]) == 0);
		UT_ASSERT(nvobj::string_view(it->value())
				  .compare(s_elements[i - 1]) == 0);
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

void
test_assign_inline_string(nvobj::pool<root> &pop)
{
	const std::string test_value = "value";

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_str = nvobj::make_persistent<container_string>();
		r->radix_str->try_emplace("key", test_value);
	});

	std::string new_value = "";
	for (int i = 0; i < 1000; i++) {
		new_value += "x";
		r->radix_str->find("key").assign_val(new_value);
	}

	UT_ASSERT(nvobj::string_view(r->radix_str->find("key")->value())
			  .compare(new_value) == 0);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_string>(r->radix_str);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

void
test_inline_string_u8t_key(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_inline_s_u8t =
			nvobj::make_persistent<container_inline_s_u8t>();
	});
	auto &m = *r->radix_inline_s_u8t;

	UT_ASSERT(m.size() == 0);

	for (unsigned i = 0; i < 10; i++) {
		auto key = std::basic_string<uint8_t>(i + 10, 99);
		auto ret = m.try_emplace(
			key, std::basic_string<uint8_t>({uint8_t(i)}));
		UT_ASSERT(ret.second);
		UT_ASSERT(key.compare(ret.first->key().data()) == 0);
		UT_ASSERT(ret.first->value() ==
			  std::basic_string<uint8_t>({uint8_t(i)}));
		UT_ASSERT(m.size() == i + 1);
	}

	for (unsigned i = 0; i < 10; i++) {
		auto key = std::basic_string<uint8_t>(i + 10, 99);
		auto ret = m.insert_or_assign(
			key, std::basic_string<uint8_t>({uint8_t(i + 1)}));
		UT_ASSERT(!ret.second);
		UT_ASSERT(key.compare(ret.first->key().data()) == 0);
		UT_ASSERT(ret.first->value() ==
			  std::basic_string<uint8_t>({uint8_t(i + 1)}));
		UT_ASSERT(m.size() == 10);
	}

	auto key = std::basic_string<uint8_t>(15, 99);
	auto it = m.find(key);
	UT_ASSERT(key.compare(it->key().data()) == 0);
	UT_ASSERT(it->value() == std::basic_string<uint8_t>({uint8_t(6)}));

	it = m.erase(it);
	UT_ASSERT(std::basic_string<uint8_t>(16, 99).compare(
			  it->key().data()) == 0);
	UT_ASSERT(it->value() == std::basic_string<uint8_t>({uint8_t(7)}));

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_inline_s_u8t>(
			r->radix_inline_s_u8t);
	});

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

void
test_inline_string_wchart_key(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_inline_s_wchart =
			nvobj::make_persistent<container_inline_s_wchart>();
	});
	auto &m = *r->radix_inline_s_wchart;

	UT_ASSERT(m.size() == 0);

	auto key1 = std::basic_string<wchar_t>(1, 256);
	auto key2 = std::basic_string<wchar_t>(1, 0);
	m.try_emplace(key1, 256U);
	m.try_emplace(key2, 0U);
	UT_ASSERT(m.size() == 2);
	auto it = m.find(key1);
	UT_ASSERT(it->value() == 256U);
	it = m.find(key2);
	UT_ASSERT(it->value() == 0U);

	key1 = std::basic_string<wchar_t>(10, 257);
	key2 = std::basic_string<wchar_t>(10, 1);
	m.try_emplace(key1, 999U);
	m.try_emplace(key2, 100U);
	UT_ASSERT(m.size() == 4);
	it = m.find(key1);
	UT_ASSERT(it->value() == 999U);
	it = m.find(key2);
	UT_ASSERT(it->value() == 100U);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<container_inline_s_wchart>(
			r->radix_inline_s_wchart);
	});
}

void
test_remove_inserted(nvobj::pool<root> &pop)
{
	const size_t NUM_ITER = 100;

	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_str = nvobj::make_persistent<container_string>();
	});

	/* remove element which was just inserted */
	nvobj::transaction::run(pop, [&] {
		for (size_t i = 0; i < NUM_ITER; i++) {
			UT_ASSERT(r->radix_str
					  ->emplace(std::to_string(i),
						    std::to_string(i))
					  .second);
			UT_ASSERT(r->radix_str->erase(std::to_string(i)));
		}
	});

	/* insert some initial elements */
	nvobj::transaction::run(pop, [&] {
		for (size_t i = 0; i < 5; i++)
			UT_ASSERT(r->radix_str
					  ->emplace("init" + std::to_string(i),
						    std::to_string(i))
					  .second);
	});

	/* remove element which was just inserted */
	nvobj::transaction::run(pop, [&] {
		for (size_t i = 0; i < NUM_ITER; i++) {
			UT_ASSERT(r->radix_str
					  ->emplace(std::to_string(i),
						    std::to_string(i))
					  .second);
			UT_ASSERT(r->radix_str->erase(std::to_string(i)));
		}
	});

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

	std::random_device rd;
	auto seed = rd();
	std::cout << "rand seed: " << seed << std::endl;
	generator = std::mt19937_64(seed);

	test_ref_stability(pop);
	test_iterators(pop);
	test_find(pop);
	test_erase(pop);
	test_binary_keys(pop);
	test_pre_post_fixes(pop);
	test_assign_inline_string(pop);
	test_compression(pop);
	test_inline_string_u8t_key(pop);
	test_inline_string_wchart_key(pop);
	test_remove_inserted(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
