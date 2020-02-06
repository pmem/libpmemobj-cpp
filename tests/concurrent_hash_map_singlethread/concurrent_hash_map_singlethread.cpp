// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018-2020, Intel Corporation */

/*
 * concurrent_hash_map.cpp -- pmem::obj::concurrent_hash_map test
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <thread>
#include <vector>

#include <libpmemobj++/container/concurrent_hash_map.hpp>
#include <libpmemobj++/container/string.hpp>

#define LAYOUT "concurrent_hash_map"

namespace nvobj = pmem::obj;

namespace
{

typedef nvobj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int>>
	persistent_map_type;

typedef persistent_map_type::value_type value_type;

struct move_element {
	move_element(int val) noexcept : val(val)
	{
	}

	move_element(const move_element &) = delete;

	move_element &operator=(const move_element &) = delete;

	move_element(move_element &&e) noexcept : val(e.val)
	{
	}

	move_element &
	operator=(move_element &&e)
	{
		val = e.val;
		return *this;
	}

	nvobj::p<int> val;
};

class MyLong {
public:
	MyLong() : val(0)
	{
	}

	explicit MyLong(long v) : val(v)
	{
	}

	MyLong(int v) : val(v)
	{
		UT_ASSERT(pmemobj_pool_by_ptr(this) != nullptr);
	}

	long
	get_val() const
	{
		return val.get_ro();
	}

	MyLong &
	operator=(long other)
	{
		val = other;
		return *this;
	}

	bool
	operator==(const MyLong &other) const
	{
		return val.get_ro() == other.val.get_ro();
	}

	bool
	operator==(int other) const
	{
		return val.get_ro() == long(other);
	}

private:
	nvobj::p<long> val;
};

class TransparentKeyEqual {
public:
	template <typename M, typename U>
	bool
	operator()(const M &lhs, const U &rhs) const
	{
		return lhs == rhs;
	}
};

class string_hasher {
	/* hash multiplier used by fibonacci hashing */
	static const size_t hash_multiplier = 11400714819323198485ULL;

public:
	using transparent_key_equal = TransparentKeyEqual;

	size_t
	operator()(const nvobj::string &str) const
	{
		return hash(str.c_str(), str.size());
	}

	size_t
	operator()(const std::string &str) const
	{
		return hash(str.c_str(), str.size());
	}

private:
	size_t
	hash(const char *str, size_t size) const
	{
		size_t h = 0;
		for (size_t i = 0; i < size; ++i) {
			h = static_cast<size_t>(str[i]) ^ (h * hash_multiplier);
		}
		return h;
	}
};

typedef nvobj::concurrent_hash_map<nvobj::p<int>, move_element>
	persistent_map_move_type;

typedef persistent_map_move_type::value_type value_move_type;

typedef nvobj::concurrent_hash_map<nvobj::string, nvobj::p<int>, string_hasher>
	persistent_map_hetero_type;

typedef nvobj::concurrent_hash_map<nvobj::string, nvobj::string, string_hasher>
	persistent_map_str_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> map1;
	nvobj::persistent_ptr<persistent_map_type> map2;

	nvobj::persistent_ptr<persistent_map_move_type> map_move;
	nvobj::persistent_ptr<persistent_map_hetero_type> map_hetero;
	nvobj::persistent_ptr<persistent_map_str_type> map_str;

	nvobj::persistent_ptr<nvobj::string> tmp;
};

void
verify_elements(persistent_map_type &map, size_t elements)
{
	UT_ASSERT(map.size() == elements);

	for (int i = 0; i < static_cast<int>(elements); i++) {
		UT_ASSERT(map.count(i) == 1);
	}
}

/**
 * Wrapper around PMDK allocator
 * @throw std::bad_alloc on allocation failure.
 */
template <typename T, typename U, typename... Args>
void
tx_alloc_wrapper(nvobj::pool_base &pop, nvobj::persistent_ptr<U> &ptr,
		 Args &&... args)
{
	try {
		nvobj::transaction::manual tx(pop);
		ptr = nvobj::make_persistent<T>(std::forward<Args>(args)...);
		nvobj::transaction::commit();
	} catch (...) {
		throw std::bad_alloc();
	}
}

/*
 * ctor_test -- (internal) test constrcutors
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
ctor_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	tx_alloc_wrapper<persistent_map_type>(pop, map1, size_t(10));
	map1->runtime_initialize();

	UT_ASSERT(map1->bucket_count() >= 10);
	UT_ASSERT(map1->empty());

	for (int i = 0; i < 300; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	tx_alloc_wrapper<persistent_map_type>(pop, map2, map1->begin(),
					      map1->end());
	map2->runtime_initialize();

	UT_ASSERT(!map2->empty());
	UT_ASSERT(map1->size() == map2->size());

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	tx_alloc_wrapper<persistent_map_type>(pop, map2, *map1);
	map2->runtime_initialize();

	UT_ASSERT(map1->size() == map2->size());

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	tx_alloc_wrapper<persistent_map_type>(pop, map2, std::move(*map1));
	map2->runtime_initialize();

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	tx_alloc_wrapper<persistent_map_type>(
		pop, map2,
		std::initializer_list<value_type>{value_type(0, 0),
						  value_type(1, 1)});
	map2->runtime_initialize();

	verify_elements(*map2, 2);

	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * assignment_test -- (internal) test assignment operators
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
assignment_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);
	tx_alloc_wrapper<persistent_map_type>(pop, map2);

	map1->runtime_initialize();
	map2->runtime_initialize();

	UT_ASSERT(map1->empty());

	for (int i = 0; i < 50; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	for (int i = 0; i < 300; i++) {
		UT_ASSERT(map2->insert(value_type(i, i)) == true);
	}

	*map1 = *map2;

	verify_elements(*map1, 300);

	for (int i = 300; i < 350; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	verify_elements(*map1, 350);
	verify_elements(*map2, 300);

	map2->clear();

	*map1 = *map2;

	UT_ASSERT(map1->size() == 0);
	UT_ASSERT(std::distance(map1->begin(), map1->end()) == 0);
	UT_ASSERT(map2->size() == 0);
	UT_ASSERT(std::distance(map2->begin(), map2->end()) == 0);

	for (int i = 0; i < 350; i++) {
		UT_ASSERT(map1->count(i) == 0);
		UT_ASSERT(map2->count(i) == 0);
	}

	for (int i = 0; i < 100; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	verify_elements(*map1, 100);
	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * swap_test -- (internal) test swap method
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
swap_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);
	tx_alloc_wrapper<persistent_map_type>(pop, map2);

	map1->runtime_initialize();
	map2->runtime_initialize();

	for (int i = 0; i < 50; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	for (int i = 0; i < 300; i++) {
		UT_ASSERT(map2->insert(value_type(i, i)) == true);
	}

	map1->swap(*map2);
	verify_elements(*map1, 300);
	verify_elements(*map2, 50);

	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * access_test -- (internal) test access methods
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
access_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);

	map1->runtime_initialize();

	for (int i = 0; i < 100; i++) {
		UT_ASSERT(map1->insert(value_type(i, i)) == true);
	}

	UT_ASSERT(map1->begin() ==
		  const_cast<const persistent_map_type *>(map1.get())->begin());
	UT_ASSERT(map1->end() ==
		  const_cast<const persistent_map_type *>(map1.get())->end());

	int i = 0;
	auto it = map1->begin();
	auto const_it =
		const_cast<const persistent_map_type *>(map1.get())->begin();
	while (it != map1->end()) {
		UT_ASSERT(it->first == const_it->first);
		UT_ASSERT(it->second == const_it->second);

		i++;
		it++;
		const_it++;
	}

	UT_ASSERT(static_cast<size_t>(i) == map1->size());

	pmem::detail::destroy<persistent_map_type>(*map1);
}

/*
 * insert_test -- (internal) test insert methods
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map_move = pop.root()->map_move;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);

	map1->runtime_initialize();

	{
		typename persistent_map_type::accessor accessor;
		UT_ASSERTeq(map1->insert(accessor, value_type(1, 1)), true);

		UT_ASSERTeq(accessor->first, 1);
		UT_ASSERTeq(accessor->second, 1);
	}

	{
		typename persistent_map_type::const_accessor accessor;
		UT_ASSERTeq(map1->insert(accessor, value_type(2, 2)), true);

		UT_ASSERTeq(accessor->first, 2);
		UT_ASSERTeq(accessor->second, 2);
	}

	tx_alloc_wrapper<persistent_map_move_type>(pop, map_move);

	map_move->runtime_initialize();

	{
		typename persistent_map_move_type::accessor accessor;
		value_move_type e(3, 3);

		UT_ASSERTeq(map_move->insert(accessor, std::move(e)), true);

		UT_ASSERTeq(accessor->first, 3);
		UT_ASSERTeq(accessor->second.val, 3);
	}

	{
		typename persistent_map_move_type::const_accessor accessor;
		value_move_type e(4, 4);

		UT_ASSERTeq(map_move->insert(accessor, std::move(e)), true);

		UT_ASSERTeq(accessor->first, 4);
		UT_ASSERTeq(accessor->second.val, 4);
	}

	{
		value_move_type e(5, 5);

		UT_ASSERTeq(map_move->insert(std::move(e)), true);

		persistent_map_move_type::accessor accessor;
		UT_ASSERT(map_move->find(accessor, 5) == true);

		UT_ASSERTeq(accessor->first, 5);
		UT_ASSERTeq(accessor->second.val, 5);
	}

	{
		value_move_type e(6, 6);

		UT_ASSERTeq(map_move->insert(std::move(e)), true);

		persistent_map_move_type::const_accessor accessor;
		UT_ASSERT(map_move->find(accessor, 6) == true);

		UT_ASSERTeq(accessor->first, 6);
		UT_ASSERTeq(accessor->second.val, 6);
	}

	{
		std::vector<value_type> v = {value_type(11, 11),
					     value_type(12, 12),
					     value_type(13, 13)};

		map1->insert(v.begin(), v.end());

		for (auto &e : v)
			UT_ASSERTeq(map1->count(e.first), 1);
	}

	{
		map1->insert(std::initializer_list<value_type>{
			value_type(21, 21), value_type(22, 22)});

		UT_ASSERTeq(map1->count(21), 1);
		UT_ASSERTeq(map1->count(22), 1);
	}

	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_move_type>(*map_move);
}

/*
 * hetero_test -- (internal) test heterogeneous count/find/erase methods
 * pmem::obj::concurrent_hash_map<nvobj::string, nvobj::p<int>, string_hasher >
 */
void
hetero_test(nvobj::pool<root> &pop)
{
	auto &map = pop.root()->map_hetero;
	auto &map_str = pop.root()->map_str;

	tx_alloc_wrapper<persistent_map_hetero_type>(pop, map);
	tx_alloc_wrapper<persistent_map_str_type>(pop, map_str);

	pmem::obj::transaction::run(pop, [&] {
		pop.root()->tmp =
			pmem::obj::make_persistent<pmem::obj::string>("123");
	});

	map->runtime_initialize();
	map_str->runtime_initialize();

	for (long i = 0; i < 100; ++i) {
		map->insert_or_assign(std::to_string(i), i);

		map_str->insert_or_assign(std::to_string(i), std::to_string(i));
	}

	for (int i = 0; i < 100; ++i) {
		UT_ASSERTeq(map->count(std::to_string(i)), 1);
		UT_ASSERTeq(map_str->count(std::to_string(i)), 1);
	}

	for (int i = 0; i < 100; ++i) {
		persistent_map_hetero_type::accessor accessor1;
		auto val = std::to_string(i);
		UT_ASSERT(map->find(accessor1, val));
		UT_ASSERT(val == accessor1->first);
		UT_ASSERT(i == accessor1->second);

		persistent_map_str_type::accessor accessor2;
		UT_ASSERT(map_str->find(accessor2, val));
		UT_ASSERT(val == accessor2->first);
		UT_ASSERT(std::to_string(i) == accessor2->second);
	}

	for (long i = 0; i < 100; ++i) {
		map->insert_or_assign(std::to_string(i), i + 1);
		map_str->insert_or_assign(std::to_string(i),
					  std::to_string(i + 1));
	}

	for (int i = 0; i < 100; ++i) {
		persistent_map_hetero_type::const_accessor accessor1;
		auto val = std::to_string(i);
		UT_ASSERT(map->find(accessor1, val));
		UT_ASSERT(val == accessor1->first);
		UT_ASSERT(i + 1 == accessor1->second);

		persistent_map_str_type::const_accessor accessor2;
		UT_ASSERT(map_str->find(accessor2, val));
		UT_ASSERT(val == accessor2->first);
		UT_ASSERT(std::to_string(i + 1) == accessor2->second);
	}

	for (int i = 0; i < 100; ++i) {
		UT_ASSERT(map->erase(std::to_string(i)));
		UT_ASSERT(map_str->erase(std::to_string(i)));
	}

	for (int i = 0; i < 100; ++i) {
		UT_ASSERTeq(map->count(std::to_string(i)), 0);
		UT_ASSERTeq(map_str->count(std::to_string(i)), 0);
	}

	{
		persistent_map_str_type::const_accessor accessor;
		map_str->insert(accessor, *(pop.root()->tmp));
		UT_ASSERT(map_str->count(*(pop.root()->tmp)) == 1);
	}

	pmem::obj::transaction::run(pop, [&] {
		pmem::obj::delete_persistent<pmem::obj::string>(
			pop.root()->tmp);
	});
}

/*
 * iterator_test -- (internal) test iterators
 * pmem::obj::concurrent_hash_map<nvobj::p<int>, nvobj::p<int> >
 */
void
iterator_test(nvobj::pool<root> &pop)
{
	auto &map = pop.root()->map1;
	tx_alloc_wrapper<persistent_map_type>(pop, map);

	{
		persistent_map_type::iterator i = map->begin();
		persistent_map_type::iterator j = map->end();
		UT_ASSERT(std::distance(i, j) == 0);
		UT_ASSERT(i == j);
	}
	{
		persistent_map_type::const_iterator i = map->begin();
		persistent_map_type::const_iterator j = map->end();
		UT_ASSERT(std::distance(i, j) == 0);
		UT_ASSERT(i == j);
	}
	{
		persistent_map_type::const_iterator i =
			const_cast<std::add_const<decltype(map)>::type>(map)
				->begin();
		persistent_map_type::const_iterator j =
			const_cast<std::add_const<decltype(map)>::type>(map)
				->end();
		UT_ASSERT(std::distance(i, j) == 0);
		UT_ASSERT(i == j);
		UT_ASSERT(i == map->end());
	}
	{
		persistent_map_type::iterator i;
		persistent_map_type::const_iterator j;
		(void)i;
		(void)j;
	}

	pmem::detail::destroy<persistent_map_type>(*map);
}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	const char *path = argv[1];

	nvobj::pool<root> pop;

	try {
		pop = nvobj::pool<root>::create(
			path, LAYOUT, PMEMOBJ_MIN_POOL * 20, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	ctor_test(pop);
	assignment_test(pop);
	access_test(pop);
	swap_test(pop);
	insert_test(pop);
	hetero_test(pop);
	iterator_test(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
