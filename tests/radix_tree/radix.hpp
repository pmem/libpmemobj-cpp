// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020-2021, Intel Corporation */

#include <functional>
#include <libpmemobj.h>
#include <random>

#include "transaction_helpers.hpp"
#include "unittest.hpp"

#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/experimental/radix_tree.hpp>

static std::mt19937_64 generator;

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

using cntr_int =
	nvobjex::radix_tree<nvobjex::inline_string, nvobj::p<unsigned>>;
using cntr_string =
	nvobjex::radix_tree<nvobjex::inline_string, nvobjex::inline_string>;

using cntr_int_int = nvobjex::radix_tree<unsigned, nvobj::p<unsigned>,
					 nvobjex::bytes_view<unsigned>, false>;
using cntr_int_string = nvobjex::radix_tree<unsigned, nvobjex::inline_string>;

using cntr_inline_s_wchart =
	nvobjex::radix_tree<nvobjex::basic_inline_string<wchar_t>,
			    nvobj::p<unsigned>>;
using cntr_inline_s_wchart_wchart =
	nvobjex::radix_tree<nvobjex::basic_inline_string<wchar_t>,
			    nvobjex::basic_inline_string<wchar_t>>;
using cntr_inline_s_u8t =
	nvobjex::radix_tree<nvobjex::basic_inline_string<uint8_t>,
			    nvobjex::basic_inline_string<uint8_t>>;

using cntr_int_mt =
	nvobjex::radix_tree<nvobjex::inline_string, nvobj::p<unsigned>,
			    nvobjex::bytes_view<nvobjex::inline_string>, true>;
using cntr_string_mt =
	nvobjex::radix_tree<nvobjex::inline_string, nvobjex::inline_string,
			    nvobjex::bytes_view<nvobjex::inline_string>, true>;

using cntr_int_int_mt =
	nvobjex::radix_tree<unsigned, nvobj::p<unsigned>,
			    nvobjex::bytes_view<unsigned>, true>;
using cntr_int_string_mt =
	nvobjex::radix_tree<unsigned, nvobjex::inline_string,
			    nvobjex::bytes_view<unsigned>, true>;

using cntr_inline_s_wchart_mt = nvobjex::radix_tree<
	nvobjex::basic_inline_string<wchar_t>, nvobj::p<unsigned>,
	nvobjex::bytes_view<nvobjex::basic_inline_string<wchar_t>>, true>;
using cntr_inline_s_wchart_wchart_mt = nvobjex::radix_tree<
	nvobjex::basic_inline_string<wchar_t>,
	nvobjex::basic_inline_string<wchar_t>,
	nvobjex::bytes_view<nvobjex::basic_inline_string<wchar_t>>, true>;
using cntr_inline_s_u8t_mt = nvobjex::radix_tree<
	nvobjex::basic_inline_string<uint8_t>,
	nvobjex::basic_inline_string<uint8_t>,
	nvobjex::bytes_view<nvobjex::basic_inline_string<uint8_t>>, true>;

struct root {
	nvobj::persistent_ptr<cntr_int> radix_int;
	nvobj::persistent_ptr<cntr_string> radix_str;

	nvobj::persistent_ptr<cntr_int_int> radix_int_int;
	nvobj::persistent_ptr<cntr_int_string> radix_int_str;

	nvobj::persistent_ptr<cntr_inline_s_wchart> radix_inline_s_wchart;
	nvobj::persistent_ptr<cntr_inline_s_wchart_wchart>
		radix_inline_s_wchart_wchart;
	nvobj::persistent_ptr<cntr_inline_s_u8t> radix_inline_s_u8t;

	nvobj::persistent_ptr<cntr_int_mt> radix_int_mt;
	nvobj::persistent_ptr<cntr_string_mt> radix_str_mt;

	nvobj::persistent_ptr<cntr_int_int_mt> radix_int_int_mt;
	nvobj::persistent_ptr<cntr_int_string_mt> radix_int_str_mt;

	nvobj::persistent_ptr<cntr_inline_s_wchart_mt> radix_inline_s_wchart_mt;
	nvobj::persistent_ptr<cntr_inline_s_wchart_wchart_mt>
		radix_inline_s_wchart_wchart_mt;
	nvobj::persistent_ptr<cntr_inline_s_u8t_mt> radix_inline_s_u8t_mt;
};

/* Helper functions to access key/value of different types */
template <typename Container,
	  typename Enable = typename std::enable_if<
		  std::is_same<typename Container::mapped_type,
			       nvobj::p<unsigned>>::value>::type>
typename Container::mapped_type
value(unsigned v, int repeats = 1)
{
	(void)repeats;
	return v;
}

template <typename Container,
	  typename Enable = typename std::enable_if<std::is_same<
		  typename Container::mapped_type,
		  typename nvobjex::basic_inline_string<
			  typename Container::mapped_type::value_type>>::
							    value>::type>
std::basic_string<typename Container::mapped_type::value_type>
value(unsigned v, int repeats = 1)
{
	using CharT = typename Container::mapped_type::value_type;

	auto str = std::to_string(v);
	auto s = std::basic_string<CharT>{};
	for (int i = 0; i < repeats; i++) {
		s += std::basic_string<CharT>(str.begin(), str.end());
	}

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
	  typename Enable = typename std::enable_if<std::is_same<
		  typename Container::key_type,
		  typename nvobjex::basic_inline_string<
			  typename Container::key_type::value_type>>::value>::
		  type>
std::basic_string<typename Container::key_type::value_type>
key(unsigned v)
{
	using CharT = typename Container::key_type::value_type;

	auto s = std::to_string(v);

	return std::basic_string<CharT>(s.begin(), s.end());
}

template <typename CharT, typename Traits>
bool
operator==(pmem::obj::basic_string_view<CharT, Traits> lhs,
	   const std::basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) == 0;
}

template <typename CharT, typename Traits>
bool
operator==(pmem::obj::experimental::basic_inline_string<CharT, Traits> &lhs,
	   const std::basic_string<CharT, Traits> &rhs)
{
	return pmem::obj::basic_string_view<CharT, Traits>(lhs.data(),
							   lhs.size())
		       .compare(rhs) == 0;
}

template <typename CharT, typename Traits>
bool
operator!=(pmem::obj::basic_string_view<CharT, Traits> lhs,
	   const std::basic_string<CharT, Traits> &rhs)
{
	return lhs.compare(rhs) != 0;
}

template <typename CharT, typename Traits>
bool
operator!=(pmem::obj::experimental::basic_inline_string<CharT, Traits> &lhs,
	   const std::basic_string<CharT, Traits> &rhs)
{
	return pmem::obj::basic_string_view<CharT, Traits>(lhs.data(),
							   lhs.size())
		       .compare(rhs) != 0;
}

/* verify all elements in container, using lower_/upper_bound and find */
template <typename Container, typename K, typename F>
void
verify_elements(nvobj::persistent_ptr<Container> ptr, unsigned count, K &&key_f,
		F &&value_f)
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

/* run 1 thread with modifications (erase/write/etc.) and multiple threads with
 * reads */
template <typename ModifyF, typename ReadF>
static void
parallel_modify_read(ModifyF modifier, std::vector<ReadF> &readers,
		     size_t n_readers)
{
	parallel_exec(n_readers + 1, [&](size_t thread_id) {
		if (thread_id == 0) {
			modifier();
		} else {
			readers[(thread_id - 1) % readers.size()]();
		}
	});
}

/* each test suite should initialize generator at the beginning */
void
init_random()
{
	std::random_device rd;
	auto seed = rd();
	std::cout << "rand seed: " << seed << std::endl;
	generator = std::mt19937_64(seed);
}

template <typename Container>
static void
init_container(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr,
	       const size_t initial_elements, const size_t value_repeats = 1,
	       bool rand_keys = false)
{
	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<Container>(); });

	for (size_t i = 0; i < initial_elements; ++i) {
		auto k = key<Container>(i);
		if (rand_keys) {
			k = key<Container>(static_cast<unsigned>(generator()));
		}
		ptr->emplace(k, value<Container>(i, value_repeats));
	}
}
