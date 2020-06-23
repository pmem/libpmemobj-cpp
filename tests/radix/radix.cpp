// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "transaction_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/radix.hpp>

#include <functional>

#include <libpmemobj.h>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container_int =
	nvobjex::radix_tree<nvobjex::inline_string, nvobj::p<int>>;
using container_string =
	nvobjex::radix_tree<nvobjex::inline_string, nvobjex::inline_string>;

using container_int_int = nvobjex::radix_tree<int, nvobj::p<int>>;
using container_int_string = nvobjex::radix_tree<int, nvobjex::inline_string>;

struct root {
	nvobj::persistent_ptr<container_int> radix_int;
	nvobj::persistent_ptr<container_string> radix_str;

	nvobj::persistent_ptr<container_int_int> radix_int_int;
	nvobj::persistent_ptr<container_int_string> radix_int_str;
};

template <typename Container,
	  typename Enable = typename std::enable_if<std::is_same<
		  typename Container::mapped_type, nvobj::p<int>>::value>::type>
typename Container::mapped_type
value(int v, int repeats = 1)
{
	(void)repeats;
	return v;
}

template <typename Container,
	  typename Enable = typename std::enable_if<
		  std::is_same<typename Container::mapped_type,
			       nvobjex::inline_string>::value>::type>
std::string
value(int v, int repeats = 1)
{
	auto s = std::string("");
	for (int i = 0; i < repeats; i++)
		s += std::to_string(v);

	return s;
}

template <typename Container,
	  typename Enable = typename std::enable_if<
		  std::is_same<typename Container::key_type, int>::value>::type>
typename Container::key_type
key(int v)
{
	return v;
}

template <typename Container,
	  typename Enable = typename std::enable_if<
		  std::is_same<typename Container::key_type,
			       nvobjex::inline_string>::value>::type>
std::string
key(int v)
{
	return std::to_string(v);
}

bool
operator==(pmem::obj::string_view lhs, const std::string &rhs)
{
	return lhs.compare(rhs) == 0;
}

template <typename Container, typename F>
void
verify_elements(nvobj::persistent_ptr<Container> ptr, int count, F &&value_f)
{
	UT_ASSERTeq(ptr->size(), static_cast<size_t>(count));
	for (int i = 0; i < count; i++) {
		auto it = ptr->find(key<Container>(i));

		UT_ASSERT((*it).first == key<Container>(i));
		UT_ASSERT((*it).second == value_f(i));
	}
}

namespace
{
void
test_iterators(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	nvobj::transaction::run(pop, [&] {
		r->radix_int = nvobj::make_persistent<container_int>();
		r->radix_int->try_emplace("", 0);
		r->radix_int->try_emplace("ab", 1);
		r->radix_int->try_emplace("ba", 2);
		r->radix_int->try_emplace("a", 3);
		r->radix_int->try_emplace("b", 4);

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

template <typename Container>
void
test_emplace(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<Container>(); });

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		auto it = ptr->try_emplace(key<Container>(0),
					   value<Container>(0));
		UT_ASSERT(it.second);
		UT_ASSERT(it.first.key() == key<Container>(0));
		UT_ASSERT(it.first.value() == value<Container>(0));

		UT_ASSERTeq(ptr->size(), 1);
	});

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		for (int i = 0; i < 1024; i++) {
			auto it = ptr->try_emplace(key<Container>(i),
						   value<Container>(i));
			UT_ASSERT(it.second);
			UT_ASSERT(it.first.key() == key<Container>(i));
			UT_ASSERT(it.first.value() == value<Container>(i));
		}

		UT_ASSERTeq(ptr->size(), 1024);
	});

	UT_ASSERTeq(ptr->size(), 0);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_assign(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](int v) { return value<Container>(v, ValueRepeats); };

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		for (int i = 0; i < 10; i++)
			ptr->try_emplace(key<Container>(i), value_f(i));
	});

	UT_ASSERTeq(ptr->size(), 10);

	const auto test_key = 3;
	const auto test_value = 99;

	typename Container::iterator it;

	assert_tx_abort(pop, [&] {
		it = ptr->find(key<Container>(test_key));
		it.value() = value_f(test_value);

		UT_ASSERT(it.value() == value_f(test_value));
		UT_ASSERT(ptr->find(key<Container>(test_key)).value() ==
			  value_f(test_value));
	});

	verify_elements(ptr, 10, value_f);
	UT_ASSERT(it.value() == value_f(test_key));

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_assign_internal_leaf(nvobj::pool<root> &pop,
			  nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](int v) { return value<Container>(v, ValueRepeats); };

	const auto test_value = 999;
	const auto new_value = 1000;

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		ptr->try_emplace("", value_f(test_value));
		for (size_t i = 1; i <= 10; i++)
			ptr->try_emplace(std::string(i, 'a'),
					 value_f(test_value));
	});

	UT_ASSERTeq(ptr->size(), 11);

	typename Container::iterator it;

	assert_tx_abort(pop, [&] {
		it = ptr->find("");
		it.value() = value_f(new_value);

		UT_ASSERT(it.value() == value_f(new_value));
		UT_ASSERT(ptr->find("").value() == value_f(new_value));
	});

	UT_ASSERTeq(ptr->size(), 11);
	UT_ASSERT(it.value() == value_f(test_value));
	UT_ASSERT(ptr->find("").value() == value_f(test_value));

	assert_tx_abort(pop, [&] {
		it = ptr->find("aaa");
		it.value() = value_f(new_value);

		UT_ASSERT(it.value() == value_f(new_value));
		UT_ASSERT(ptr->find("aaa").value() == value_f(new_value));
	});

	UT_ASSERTeq(ptr->size(), 11);
	UT_ASSERT(it.value() == value_f(test_value));
	UT_ASSERT(ptr->find("aaa").value() == value_f(test_value));

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_assign_root(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](int v) { return value<Container>(v, ValueRepeats); };

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		ptr->try_emplace(key<Container>(0), value_f(0));
	});

	UT_ASSERTeq(ptr->size(), 1);

	typename Container::iterator it;

	assert_tx_abort(pop, [&] {
		it = ptr->find(key<Container>(0));
		it.value() = value_f(1);

		UT_ASSERT(it.value() == value_f(1));
		UT_ASSERT(ptr->find(key<Container>(0)).value() == value_f(1));
	});

	verify_elements(ptr, 1, value_f);

	UT_ASSERT(it.value() == value_f(0));
	UT_ASSERT(ptr->find(key<Container>(0)).value() == value_f(0));

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_erase(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](int v) { return value<Container>(v, ValueRepeats); };

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		for (int i = 0; i < 1024; i++)
			ptr->try_emplace(key<Container>(i), value_f(i));
	});

	UT_ASSERTeq(ptr->size(), 1024);

	assert_tx_abort(pop, [&] {
		for (int i = 0; i < 1024; i++) {
			UT_ASSERTeq(ptr->size(), static_cast<size_t>(1024 - i));
			UT_ASSERTeq(ptr->erase(key<Container>(i)), 1);
		}
	});

	verify_elements(ptr, 1024, value_f);

	assert_tx_abort(pop, [&] {
		for (int i = 1024; i > 0; i--) {
			UT_ASSERTeq(ptr->size(), static_cast<size_t>(i));
			UT_ASSERTeq(ptr->erase(key<Container>(i - 1)), 1);
		}
	});

	verify_elements(ptr, 1024, value_f);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_erase_internal(nvobj::pool<root> &pop,
		    nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](int v) { return value<Container>(v, ValueRepeats); };

	const auto test_value = 999;

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		ptr->try_emplace("", value_f(test_value));
		for (size_t i = 1; i <= 10; i++)
			ptr->try_emplace(std::string(i, 'a'),
					 value_f(test_value));
	});

	UT_ASSERTeq(ptr->size(), 11);

	assert_tx_abort(pop, [&] {
		for (size_t i = 1; i <= 10; i++) {
			UT_ASSERTeq(ptr->size(), 12 - i);
			UT_ASSERTeq(ptr->erase(std::string(i, 'a')), 1);
		}

		UT_ASSERTeq(ptr->erase(""), 1);
	});

	UT_ASSERTeq(ptr->size(), 11);
	UT_ASSERT(ptr->find("") != ptr->end());
	for (size_t i = 1; i <= 10; i++)
		UT_ASSERT(ptr->find(std::string(i, 'a')) != ptr->end());

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
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
		pop = nvobj::pool<struct root>::create(path, "map_tx",
						       10 * PMEMOBJ_MIN_POOL,
						       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_iterators(pop);

	test_emplace(pop, pop.root()->radix_str);
	test_assign<container_string, 1>(pop, pop.root()->radix_str);
	test_assign<container_string, 1024>(pop, pop.root()->radix_str);
	test_assign_root<container_string, 1>(pop, pop.root()->radix_str);
	test_assign_root<container_string, 1024>(pop, pop.root()->radix_str);
	test_erase<container_string, 1024>(pop, pop.root()->radix_str);
	test_assign_internal_leaf<container_string, 1>(pop,
						       pop.root()->radix_str);
	test_assign_internal_leaf<container_string, 1024>(
		pop, pop.root()->radix_str);
	test_erase_internal<container_string, 1024>(pop, pop.root()->radix_str);

	test_emplace(pop, pop.root()->radix_int);
	test_assign<container_int, 1>(pop, pop.root()->radix_int);
	test_assign_root<container_int, 1>(pop, pop.root()->radix_int);
	test_erase<container_int, 1024>(pop, pop.root()->radix_int);
	test_assign_internal_leaf<container_int, 1>(pop, pop.root()->radix_int);
	test_erase_internal<container_int, 1024>(pop, pop.root()->radix_int);

	test_emplace(pop, pop.root()->radix_int_int);
	test_assign<container_int_int, 1>(pop, pop.root()->radix_int_int);
	test_assign_root<container_int_int, 1>(pop, pop.root()->radix_int_int);
	test_erase<container_int_int, 1>(pop, pop.root()->radix_int_int);

	test_emplace(pop, pop.root()->radix_int_str);
	test_assign<container_int_string, 1>(pop, pop.root()->radix_int_str);
	test_assign<container_int_string, 1024>(pop, pop.root()->radix_int_str);
	test_assign_root<container_int_string, 1>(pop,
						  pop.root()->radix_int_str);
	test_assign_root<container_int_string, 1024>(pop,
						     pop.root()->radix_int_str);
	test_erase<container_int_string, 1024>(pop, pop.root()->radix_int_str);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
