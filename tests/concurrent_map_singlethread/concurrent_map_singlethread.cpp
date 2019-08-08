// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/*
 * concurrent_map_singlethread.cpp -- pmem::obj::experimental::concurrent_map
 * basic tests
 *
 */

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include <libpmemobj++/experimental/concurrent_map.hpp>

#define LAYOUT "concurrent_map"

namespace nvobj = pmem::obj;

namespace
{

typedef nvobj::experimental::concurrent_map<nvobj::p<int>, nvobj::p<int>>
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
		e.val = 0;
	}

	move_element &
	operator=(move_element &&e)
	{
		val = e.val;
		e.val = 0;
		return *this;
	}

	nvobj::p<int> val;
};

typedef nvobj::experimental::concurrent_map<nvobj::p<int>, move_element>
	persistent_map_move_type;

typedef persistent_map_move_type::value_type value_move_type;

struct hetero_less {
	using is_transparent = void;
	template <typename T1, typename T2>
	bool
	operator()(const T1 &lhs, const T2 &rhs) const
	{
		return lhs < rhs;
	}
};

inline std::string
gen_hetero(int v)
{
	return std::to_string(v);
}

typedef nvobj::experimental::concurrent_map<pmem::obj::string,
					    pmem::obj::string, hetero_less>
	persistent_map_string_type;

struct root {
	nvobj::persistent_ptr<persistent_map_type> map1;
	nvobj::persistent_ptr<persistent_map_type> map2;

	nvobj::persistent_ptr<persistent_map_move_type> map_move;

	nvobj::persistent_ptr<persistent_map_string_type> map_string;
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
 * ctor_test -- (internal) test constructors
 * pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
ctor_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);
	UT_ASSERT(map1->empty());
	UT_ASSERT(map1->size() == size_t(0));

	for (int i = 0; i < 300; i++) {
		auto ret = map1->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
		UT_ASSERT(ret.first->first == i);
		UT_ASSERT(ret.first->second == i);
	}

	tx_alloc_wrapper<persistent_map_type>(pop, map2, map1->begin(),
					      map1->end());

	UT_ASSERT(!map2->empty());
	UT_ASSERT(map1->size() == map2->size());

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	tx_alloc_wrapper<persistent_map_type>(pop, map2, *map1);

	UT_ASSERT(map1->size() == map2->size());

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	tx_alloc_wrapper<persistent_map_type>(pop, map2, std::move(*map1));

	verify_elements(*map2, 300);

	pmem::detail::destroy<persistent_map_type>(*map2);
	tx_alloc_wrapper<persistent_map_type>(
		pop, map2,
		std::initializer_list<value_type>{value_type(0, 0),
						  value_type(1, 1)});

	verify_elements(*map2, 2);

	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * assignment_test -- (internal) test assignment operators
 * pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
assignment_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);
	tx_alloc_wrapper<persistent_map_type>(pop, map2);

	UT_ASSERT(map1->empty());

	for (int i = 0; i < 50; i++) {
		auto ret = map1->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
	}

	verify_elements(*map1, 50);

	for (int i = 0; i < 300; i++) {
		auto ret = map2->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
	}

	*map1 = *map2;

	verify_elements(*map1, 300);

	for (int i = 300; i < 350; i++) {
		auto ret = map1->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
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
		auto ret = map1->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
	}

	verify_elements(*map1, 100);

	*map2 = std::move(*map1);

	verify_elements(*map2, 100);

	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * swap_test -- (internal) test swap method
 * pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
swap_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map2 = pop.root()->map2;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);
	tx_alloc_wrapper<persistent_map_type>(pop, map2);

	for (int i = 0; i < 50; i++) {
		auto ret = map1->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
	}

	for (int i = 0; i < 300; i++) {
		auto ret = map2->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
	}

	map1->swap(*map2);
	verify_elements(*map1, 300);
	verify_elements(*map2, 50);

	pmem::detail::destroy<persistent_map_type>(*map1);
	pmem::detail::destroy<persistent_map_type>(*map2);
}

/*
 * access_test -- (internal) test access methods
 * pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
access_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);

	for (int i = 0; i < 100; i++) {
		auto ret = map1->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
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
 * pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
insert_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;
	auto &map_move = pop.root()->map_move;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);

	{
		auto ret = map1->insert(value_type(1, 1));
		UT_ASSERTeq(ret.second, true);
		UT_ASSERTeq(ret.first->first, 1);
		UT_ASSERTeq(ret.first->second, 1);
	}

	{
		auto ret = map1->insert(value_type(2, 2));
		UT_ASSERTeq(ret.second, true);
		UT_ASSERTeq(ret.first->first, 2);
		UT_ASSERTeq(ret.first->second, 2);
	}

	tx_alloc_wrapper<persistent_map_move_type>(pop, map_move);

	{
		value_move_type e(3, 3);

		auto ret = map_move->insert(std::move(e));
		UT_ASSERTeq(ret.second, true);
		UT_ASSERTeq(ret.first->first, 3);
		UT_ASSERTeq(ret.first->second.val, 3);
	}

	{
		value_move_type e(4, 4);

		auto ret = map_move->insert(std::move(e));
		UT_ASSERTeq(ret.second, true);
		UT_ASSERTeq(ret.first->first, 4);
		UT_ASSERTeq(ret.first->second.val, 4);
	}

	{
		value_move_type e(5, 5);

		auto ret = map_move->insert(std::move(e));
		UT_ASSERTeq(ret.second, true);

		using const_iterator =
			typename persistent_map_move_type::const_iterator;
		const_iterator it = map_move->find(5);

		UT_ASSERT(it != map_move->end());

		UT_ASSERTeq(it->first, 5);
		UT_ASSERTeq(it->second.val, 5);
	}

	{
		value_move_type e(6, 6);

		auto ret = map_move->insert(std::move(e));
		UT_ASSERTeq(ret.second, true);

		using iterator = typename persistent_map_move_type::iterator;
		iterator it = map_move->find(6);

		UT_ASSERT(it != map_move->end());

		UT_ASSERTeq(it->first, 6);
		UT_ASSERTeq(it->second.val, 6);
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
 * emplace_test -- (internal) test emplace method
 */
void
emplace_test(nvobj::pool<root> &pop)
{
	auto &map = pop.root()->map1;
	auto &map_move = pop.root()->map_move;
	auto &map_string = pop.root()->map_string;

	tx_alloc_wrapper<persistent_map_type>(pop, map);

	{
		auto ret = map->emplace(value_type(1, 1));
		UT_ASSERTeq(ret.second, true);
		UT_ASSERTeq(ret.first->first, 1);
		UT_ASSERTeq(ret.first->second, 1);
	}

	{
		auto ret = map->emplace(value_type(1, 2));
		UT_ASSERTeq(ret.second, false);
		UT_ASSERTeq(ret.first->first, 1);
		UT_ASSERTeq(ret.first->second, 1);
	}

	{
		auto ret = map->emplace(2, 2);
		UT_ASSERTeq(ret.second, true);
		UT_ASSERTeq(ret.first->first, 2);
		UT_ASSERTeq(ret.first->second, 2);
	}

	{
		auto ret = map->emplace(2, 3);
		UT_ASSERTeq(ret.second, false);
		UT_ASSERTeq(ret.first->first, 2);
		UT_ASSERTeq(ret.first->second, 2);
	}

	tx_alloc_wrapper<persistent_map_move_type>(pop, map_move);

	{
		value_move_type e(3, 3);

		auto ret = map_move->emplace(std::move(e));
		UT_ASSERTeq(ret.second, true);
		UT_ASSERTeq(ret.first->first, 3);
		UT_ASSERTeq(ret.first->second.val, 3);
	}

	{
		move_element m(4);
		auto ret = map_move->emplace(4, std::move(m));
		UT_ASSERTeq(ret.second, true);
		UT_ASSERTeq(ret.first->first, 4);
		UT_ASSERTeq(ret.first->second.val, 4);
		UT_ASSERTeq(m.val, 0); // m was moved
	}

	{
		move_element m(5);
		auto ret = map_move->try_emplace(4, std::move(m));
		UT_ASSERTeq(ret.second, false);
		UT_ASSERTeq(ret.first->first, 4);
		UT_ASSERTeq(ret.first->second.val, 4);
		UT_ASSERTeq(m.val, 5); // m was not moved
	}

	tx_alloc_wrapper<persistent_map_string_type>(pop, map_string);
	{
		std::string str("foo");
		auto ret = map_string->emplace(str, str);
		UT_ASSERTeq(ret.second, true);
		UT_ASSERT(ret.first->first == str);
		UT_ASSERT(ret.first->second == str);
	}

	{
		auto ret = map_string->emplace("bar", "bar");
		UT_ASSERTeq(ret.second, true);
		UT_ASSERT(ret.first->first == "bar");
		UT_ASSERT(ret.first->second == "bar");
	}

	{
		std::string str("foo\0bar", 7);
		auto ret = map_string->emplace(
			std::piecewise_construct,
			std::forward_as_tuple(str.c_str(), str.size()),
			std::forward_as_tuple(str.c_str(), str.size()));
		UT_ASSERTeq(ret.second, true);
		UT_ASSERT(ret.first->first == str);
		UT_ASSERT(ret.first->second == str);
	}

	{
		std::string str("bar\0foo", 7);
		auto ret =
			map_string->try_emplace(str, str.c_str(), str.size());
		UT_ASSERTeq(ret.second, true);
		UT_ASSERT(ret.first->first == str);
		UT_ASSERT(ret.first->second == str);

		ret = map_string->try_emplace(str, "new_value");
		UT_ASSERTeq(ret.second, false);
		UT_ASSERT(ret.first->first == str);
		UT_ASSERT(ret.first->second == str);
	}

	pmem::detail::destroy<persistent_map_type>(*map);
	pmem::detail::destroy<persistent_map_move_type>(*map_move);
	pmem::detail::destroy<persistent_map_string_type>(*map_string);
}

template <bool is_const, typename MapType>
void
bound_helper(nvobj::persistent_ptr<MapType> &m)
{
	using iterator =
		typename std::conditional<is_const,
					  typename MapType::const_iterator,
					  typename MapType::iterator>::type;

	using map_ptr_type =
		typename std::conditional<is_const, const MapType *,
					  MapType *>::type;
	map_ptr_type map = m.get();

	for (int i = 0; i < 298; ++i) {
		iterator lb_it = map->lower_bound(i);
		iterator ub_it = map->upper_bound(i);
		UT_ASSERT(lb_it != map->end());
		UT_ASSERT(ub_it != map->end());

		std::pair<iterator, iterator> range = map->equal_range(i);

		if (i % 2 == 0) {
			UT_ASSERT(lb_it != ub_it);
			UT_ASSERTeq(lb_it->first, i);
			UT_ASSERTeq(lb_it->second, i);
			UT_ASSERTeq(ub_it->first, i + 2);
			UT_ASSERTeq(ub_it->second, i + 2);

			UT_ASSERTeq(std::distance(range.first, range.second),
				    1);
			UT_ASSERTeq(range.first->first, i);
			UT_ASSERTeq(range.first->second, i);
			UT_ASSERTeq(range.second->first, i + 2);
			UT_ASSERTeq(range.second->second, i + 2);
		} else {
			UT_ASSERT(lb_it == ub_it);
			UT_ASSERTeq(lb_it->first, i + 1);
			UT_ASSERTeq(lb_it->second, i + 1);
			UT_ASSERTeq(ub_it->first, i + 1);
			UT_ASSERTeq(ub_it->second, i + 1);

			UT_ASSERTeq(std::distance(range.first, range.second),
				    0);
			UT_ASSERTeq(range.first->first, i + 1);
			UT_ASSERTeq(range.first->second, i + 1);
			UT_ASSERTeq(range.second->first, i + 1);
			UT_ASSERTeq(range.second->second, i + 1);
		}
	}

	iterator lb_it = map->lower_bound(298);
	UT_ASSERT(lb_it != map->end());
	UT_ASSERTeq(lb_it->first, 298);
	UT_ASSERTeq(lb_it->second, 298);

	iterator ub_it = map->upper_bound(298);
	UT_ASSERT(ub_it == map->end());

	lb_it = map->lower_bound(300);
	UT_ASSERT(lb_it == map->end());

	ub_it = map->upper_bound(300);
	UT_ASSERT(ub_it == map->end());

	lb_it = map->lower_bound(-1);
	UT_ASSERT(lb_it == map->begin());
	UT_ASSERTeq(lb_it->first, 0);
	UT_ASSERTeq(lb_it->second, 0);

	ub_it = map->upper_bound(-1);
	UT_ASSERT(ub_it == map->begin());
	UT_ASSERTeq(ub_it->first, 0);
	UT_ASSERTeq(ub_it->second, 0);

	std::pair<iterator, iterator> range = map->equal_range(-1);
	UT_ASSERTeq(std::distance(range.first, range.second), 0);
	UT_ASSERT(range.first == range.second);
	UT_ASSERT(range.first == map->begin());

	range = map->equal_range(298);
	UT_ASSERTeq(std::distance(range.first, range.second), 1);
	UT_ASSERT(range.first != map->end());
	UT_ASSERT(range.second == map->end());
	UT_ASSERTeq(range.first->first, 298);
	UT_ASSERTeq(range.first->second, 298);

	range = map->equal_range(300);
	UT_ASSERTeq(std::distance(range.first, range.second), 0);
	UT_ASSERT(range.first == map->end());
	UT_ASSERT(range.second == map->end());
}

/*
 * bound_test -- (internal) test upper_bound, lower_bound and equal_range
 * methods pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
bound_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);

	for (int i = 0; i < 300; i += 2) {
		auto ret = map1->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
	}

	bound_helper<true>(map1);
	bound_helper<false>(map1);

	pmem::detail::destroy<persistent_map_type>(*map1);
}

/*
 * erase_test -- (internal) test erase methods
 * pmem::obj::concurrent_map<nvobj::p<int>, nvobj::p<int> >
 */
void
erase_test(nvobj::pool<root> &pop)
{
	auto &map1 = pop.root()->map1;

	tx_alloc_wrapper<persistent_map_type>(pop, map1);

	for (int i = 0; i < 300; ++i) {
		auto ret = map1->insert(value_type(i, i));
		UT_ASSERT(ret.second == true);
	}

	auto last = map1->find(150);
	UT_ASSERT(last != map1->end());

	auto ret = map1->unsafe_erase(map1->begin(), last);

	UT_ASSERT(map1->begin() == ret);
	UT_ASSERTeq(150, std::distance(map1->begin(), map1->end()));

	for (int i = 0; i < 150; ++i) {
		UT_ASSERT(map1->contains(i) == false);
	}

	for (int i = 150; i < 300; ++i) {
		UT_ASSERT(map1->contains(i));
	}

	UT_ASSERTeq(map1->unsafe_erase(150), size_t(1));
	UT_ASSERT(map1->find(150) == map1->end());

	persistent_map_type::const_iterator const_it = map1->find(151);
	UT_ASSERT(const_it != map1->end());
	persistent_map_type::iterator it = map1->unsafe_erase(const_it);
	UT_ASSERT(map1->contains(151) == false);
	UT_ASSERT(it != map1->end());
	UT_ASSERTeq(it->first, 152);
	UT_ASSERTeq(it->second, 152);
	it = map1->unsafe_erase(it);
	UT_ASSERT(map1->contains(152) == false);
	UT_ASSERT(it != map1->end());
	UT_ASSERTeq(it->first, 153);
	UT_ASSERTeq(it->second, 153);

	pmem::detail::destroy<persistent_map_type>(*map1);
}

template <bool is_const, typename MapType>
void
hetero_helper(nvobj::persistent_ptr<MapType> &m)
{
	using iterator =
		typename std::conditional<is_const,
					  typename MapType::const_iterator,
					  typename MapType::iterator>::type;

	using map_ptr_type =
		typename std::conditional<is_const, const MapType *,
					  MapType *>::type;

	using value_type = typename MapType::value_type;

	map_ptr_type map = m.get();

	UT_ASSERT(std::is_sorted(
		map->begin(), map->end(),
		[](const value_type &lhs, const value_type &rhs) {
			return lhs.first < rhs.first;
		}));

	for (int i = 0; i < 100; ++i) {
		UT_ASSERT(map->contains(gen_hetero(i)));
	}

	for (int i = 0; i < 100; ++i) {
		iterator it = map->find(gen_hetero(i));

		UT_ASSERT(it != map->end());
		UT_ASSERT(it->first == gen_hetero(i));
		UT_ASSERT(it->second == gen_hetero(i));
	}

	for (int i = 0; i < 99; ++i) {
		iterator lb_it = map->lower_bound(gen_hetero(i));
		iterator ub_it = map->upper_bound(gen_hetero(i));
		UT_ASSERT(lb_it != map->end());
		UT_ASSERT(ub_it != map->end());
		UT_ASSERT(lb_it != ub_it);
		UT_ASSERT(lb_it->first == gen_hetero(i));
		UT_ASSERT(lb_it->second == gen_hetero(i));

		// make sure that ub_it points to the next after lb_it
		UT_ASSERTeq(std::distance(lb_it, ub_it), 1);

		UT_ASSERT(ub_it->first == ub_it->second);

		std::pair<iterator, iterator> range =
			map->equal_range(gen_hetero(i));
		UT_ASSERTeq(std::distance(range.first, range.second), 1);
		UT_ASSERT(range.first->first == gen_hetero(i));
		UT_ASSERT(range.first->second == gen_hetero(i));
		UT_ASSERT(range.second->first == range.second->second);
	}
}

/*
 * hetero_test -- (internal) test heterogeneous contains/count/find/erase
 * methods pmem::obj::concurrent_map<pmem::obj::string, pmem::obj::string,
 * std::less<void> >
 */
void
hetero_test(nvobj::pool<root> &pop)
{
	auto &map = pop.root()->map_string;

	tx_alloc_wrapper<persistent_map_string_type>(pop, map);

	for (int i = 0; i < 100; ++i) {
		map->emplace(gen_hetero(i), gen_hetero(i));
	}

	hetero_helper<true>(map);
	hetero_helper<false>(map);

	for (int i = 0; i < 100; ++i) {
		UT_ASSERTeq(map->unsafe_erase(gen_hetero(i)), 1);
	}

	for (int i = 0; i < 100; ++i) {
		UT_ASSERTeq(map->count(gen_hetero(i)), 0);
	}

	for (int i = 0; i < 100; ++i) {
		UT_ASSERT(map->contains(gen_hetero(i)) == false);
	}

	pmem::detail::destroy<persistent_map_string_type>(*map);
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
	emplace_test(pop);
	bound_test(pop);
	erase_test(pop);
	hetero_test(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
