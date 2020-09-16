// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "transaction_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/radix_tree.hpp>

#include <functional>

#include <libpmemobj.h>

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using container_int =
	nvobjex::radix_tree<nvobjex::inline_string, nvobj::p<unsigned>>;
using container_string =
	nvobjex::radix_tree<nvobjex::inline_string, nvobjex::inline_string>;

using container_int_int =
	nvobjex::radix_tree<unsigned, nvobj::p<unsigned>>;
using container_int_string =
	nvobjex::radix_tree<unsigned, nvobjex::inline_string>;

using container_inline_s_u8t = nvobjex::radix_tree<nvobjex::basic_inline_string<uint8_t>, nvobj::p<unsigned>>;

struct root {
	nvobj::persistent_ptr<container_int> radix_int;
	nvobj::persistent_ptr<container_string> radix_str;

	nvobj::persistent_ptr<container_int_int> radix_int_int;
	nvobj::persistent_ptr<container_int_string> radix_int_str;

	nvobj::persistent_ptr<container_inline_s_u8t> radix_inline_s_u8t;
};

template <typename Container,
	  typename Enable = typename std::enable_if<std::is_same<
		  typename Container::mapped_type, nvobj::p<unsigned>>::value>::type>
typename Container::mapped_type
value(unsigned v, int repeats = 1)
{
	(void)repeats;
	return v;
}

template <typename Container,
	  typename Enable = typename std::enable_if<
		  std::is_same<typename Container::mapped_type,
			       nvobjex::inline_string>::value>::type>
std::string
value(unsigned v, int repeats = 1)
{
	auto s = std::string("");
	for (int i = 0; i < repeats; i++)
		s += std::to_string(v);

	return s;
}

template <typename Container,
	  typename Enable = typename std::enable_if<std::is_same<
		  typename Container::key_type, unsigned>::value>::type>
typename Container::key_type
key(unsigned v)
{
	return v;
}

template <typename Container,
	  typename Enable = typename std::enable_if<
		  std::is_same<typename Container::key_type,
			       nvobjex::inline_string>::value>::type>
std::string
key(unsigned v)
{
	return std::to_string(v);
}

bool
operator==(pmem::obj::string_view lhs, const std::string &rhs)
{
	return lhs.compare(rhs) == 0;
}

template <typename Container, typename K, typename F>
void
verify_elements(nvobj::persistent_ptr<Container> ptr, unsigned count, K&& key_f, F &&value_f)
{
	UT_ASSERTeq(ptr->size(), static_cast<size_t>(count));
	for (unsigned i = 0; i < count; i++) {
		auto it = ptr->find(key_f(i));
		UT_ASSERT(it->key() == key_f(i));
		UT_ASSERT(it->value() == value_f(i));

		auto lit = ptr->lower_bound(key_f(i));
		UT_ASSERT(lit->key() == key_f(i));
		UT_ASSERT(lit->value() == value_f(i));
	}

	std::vector<decltype(key_f(0))> keys;
	for (unsigned i = 0; i < count; i++)
		keys.emplace_back(key_f(i));
	std::sort(keys.begin(), keys.end());

	unsigned i = 0;
	for (auto it = ptr->begin(); it != ptr->end(); ++it, ++i) {
		UT_ASSERT(it->key() == keys[i]);
	}

	for (size_t i = 0; i < keys.size(); i++) {
		const auto &k = keys[i];

		auto uit = ptr->upper_bound(k);
		if (i == keys.size() - 1) {
			UT_ASSERT(uit == ptr->end());
		} else {
			UT_ASSERT(uit->key() == keys[i + 1]);
		}
	}
}
