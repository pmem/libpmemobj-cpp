// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2021, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

#define LAYOUT "cpp"

namespace nvobj = pmem::obj;
namespace nvobjex = pmem::obj::experimental;

namespace
{

template <typename CharT>
struct Object {
	template <typename... Args>
	Object(int d, Args &&... args) : data(d), s(std::forward<Args>(args)...)
	{
	}

	Object(const Object &o) = default;

	Object &operator=(const Object &o) = default;

	nvobj::p<int> data;
	nvobj::experimental::basic_inline_string<CharT> s;
};

template <typename CharT>
struct root {
	nvobj::persistent_ptr<Object<CharT>> o1;
	nvobj::persistent_ptr<Object<CharT>> o2;
	nvobj::persistent_ptr<Object<CharT>> o3;
};

template <typename T>
void
test_inline_string(nvobj::pool<struct root<T>> &pop)
{
	auto r = pop.root();

	const nvobjex::inline_string::size_type req_capacity = 100;
	std::string test_string1("abcd");
	std::basic_string<T> bs2(7, static_cast<T>('x'));
	std::string test_string3("abcdefgh");

	auto bs1 =
		std::basic_string<T>(test_string1.begin(), test_string1.end());

	auto bs3 =
		std::basic_string<T>(test_string3.begin(), test_string3.end());

	nvobj::basic_string_view<T> test_view(bs3.data(), bs3.length());

	std::basic_string<T> max_capacity_bs(req_capacity, static_cast<T>('a'));
	nvobj::basic_string_view<T> max_capacity_view(max_capacity_bs.data(),
						      max_capacity_bs.length());

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::standard_alloc_policy<void> allocator;
			r->o1 = static_cast<nvobj::persistent_ptr<Object<T>>>(
				allocator.allocate(sizeof(Object<T>) +
						   req_capacity * sizeof(T)));
			r->o2 = static_cast<nvobj::persistent_ptr<Object<T>>>(
				allocator.allocate(sizeof(Object<T>) +
						   req_capacity * sizeof(T)));
			r->o3 = static_cast<nvobj::persistent_ptr<Object<T>>>(
				allocator.allocate(sizeof(Object<T>) +
						   req_capacity * sizeof(T)));

			new (r->o1.get()) Object<T>(1, req_capacity);
			new (r->o2.get()) Object<T>(2, req_capacity);
			new (r->o3.get()) Object<T>(3, max_capacity_bs);

			r->o1->s.assign(bs1);
			r->o2->s.assign(bs2);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(r->o1->data, 1);
	UT_ASSERTeq(r->o2->data, 2);
	UT_ASSERTeq(r->o3->data, 3);

	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(bs1) == 0);
	UT_ASSERT(r->o1->s.size() == 4);
	UT_ASSERT(memcmp(r->o1->s.data(), bs1.data(), 4) == 0);

	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).data()[4] == '\0');
	UT_ASSERT(nvobj::basic_string_view<T>(r->o2->s).data()[7] == '\0');
	UT_ASSERT(r->o2->s.size() == 7);
	UT_ASSERT(nvobj::basic_string_view<T>(r->o2->s).compare(bs2) == 0);

	UT_ASSERT(r->o3->s.capacity() == r->o3->s.size());
	UT_ASSERT(nvobj::basic_string_view<T>(r->o3->s).compare(
			  max_capacity_bs) == 0);

	try {
		nvobj::transaction::run(pop, [&] {
			*(r->o1) = *(r->o2);
			UT_ASSERTeq(r->o1->s.compare(r->o2->s), 0);

			UT_ASSERTeq(r->o1->data, 2);
			UT_ASSERTeq(r->o2->data, 2);

			UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(
					  bs2) == 0);
			UT_ASSERT(nvobj::basic_string_view<T>(r->o2->s).compare(
					  bs2) == 0);

			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERTeq(r->o1->data, 1);
	UT_ASSERTeq(r->o2->data, 2);

	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(bs1) == 0);
	UT_ASSERT(nvobj::basic_string_view<T>(r->o2->s).compare(bs2) == 0);

	r->o1->s = bs3;
	UT_ASSERT(test_view.compare(r->o1->s) == 0);

	nvobj::basic_string_view<T> new_view = r->o1->s;
	UT_ASSERT(new_view.compare(test_view) == 0);

	try {
		std::string str(r->o1->s.capacity() + 5, 'x');
		std::basic_string<T> bs(str.begin(), str.end());
		nvobj::basic_string_view<T> v1(bs.data(), bs.length());

		nvobj::transaction::run(pop, [&] {
			r->o1->s = v1;
			UT_ASSERT(0);
		});
	} catch (std::out_of_range &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	{
		std::string empty("");
		std::basic_string<T> bs(empty.begin(), empty.end());
		r->o1->s.assign(
			nvobj::basic_string_view<T>(bs.data(), bs.length()));
	}
	try {
		nvobj::transaction::run(pop, [&] {
			std::string test_string("aaaa");
			std::basic_string<T> bs(test_string.begin(),
						test_string.end());
			r->o1->s.assign(nvobj::basic_string_view<T>(
				bs.data(), bs.length()));

			r->o1->s = r->o1->s;
			UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(
					  nvobj::basic_string_view<T>(
						  bs.data(), bs.length())) ==
				  0);

			nvobj::transaction::abort(0);
			UT_ASSERT(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	{
		std::basic_string<T> bs;
		UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(
				  nvobj::basic_string_view<T>(
					  bs.data(), bs.length())) == 0);
	}
	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).data()[0] == '\0');
	UT_ASSERT(r->o1->s.size() == 0);

	/* test operator[], at(size_t) and range(size_t, size_t)*/
	r->o1->s.assign(bs1);
	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(bs1) == 0);
	UT_ASSERT(r->o1->s.size() == 4);
	try {
		nvobj::transaction::run(pop, [&] {
			UT_ASSERT(r->o1->s[0] == bs1[0]);
			r->o1->s[0] = bs1[1];
			UT_ASSERT(r->o1->s[0] == bs1[1]);
			UT_ASSERT(r->o1->s.size() == 4);

			UT_ASSERT(r->o1->s[1] == bs1[1]);
			r->o1->s.at(1) = bs1[2];
			UT_ASSERT(r->o1->s[1] == bs1[2]);
			UT_ASSERT(r->o1->s.size() == 4);

			UT_ASSERT(r->o1->s.at(2) == bs1[2]);
			UT_ASSERT(r->o1->s.at(3) == bs1[3]);
			size_t cnt = 2;
			for (auto &c : r->o1->s.range(2, 2)) {
				c = bs1[3];
				UT_ASSERT(r->o1->s.at(cnt++) == bs1[3]);
			}
			UT_ASSERT(r->o1->s.at(0) == bs1[1]);
			UT_ASSERT(r->o1->s.at(1) == bs1[2]);
			UT_ASSERT(r->o1->s.size() == 4);

			nvobj::transaction::abort(0);
			UT_ASSERT(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(bs1) == 0);
	UT_ASSERT(r->o1->s.size() == 4);

	const auto &const_inline = r->o1->s;
	UT_ASSERT(const_inline[0] == bs1[0]);
	UT_ASSERT(const_inline.at(1) == bs1[1]);

	try {
		r->o1->s.at(5);
		UT_ASSERT(0);
	} catch (std::out_of_range &e) {
	}

	try {
		r->o1->s.range(1, 4);
		UT_ASSERT(0);
	} catch (std::out_of_range &e) {
	}

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<Object<T>>(r->o1);
		nvobj::delete_persistent<Object<T>>(r->o2);
		nvobj::delete_persistent<Object<T>>(r->o3);
	});
}

/* test if dram_inline_string can be placed on dram */
template <typename T>
void
test_dram(nvobj::pool<struct root<T>> &pop)
{
	using string_type = nvobj::experimental::basic_dram_inline_string<T>;

	constexpr size_t string_size = 20;
	typename std::aligned_storage<sizeof(string_type) +
					      (string_size + 1) * sizeof(T),
				      alignof(string_type)>::type buffer;

	std::basic_string<T> s(string_size, T('a'));

	auto dram_location = reinterpret_cast<string_type *>(&buffer);
	new (dram_location)
		string_type(nvobj::basic_string_view<T>(s.data(), s.length()));

	UT_ASSERTeq(s.length(), dram_location->size());
	UT_ASSERTeq(std::char_traits<T>::compare(
			    s.data(), dram_location->data(), s.length()),
		    0);

	dram_location->~string_type();

	new (dram_location) string_type(string_size);

	UT_ASSERTeq(dram_location->capacity(), string_size);
	UT_ASSERTeq(dram_location->size(), 0);

	/* inline_string cannot be modified on dram. */
	try {
		s = std::basic_string<T>(string_size / 2, T('b'));
		dram_location->assign(s.data());

		ASSERT_UNREACHABLE;
	} catch (pmem::pool_error &) {
	} catch (...) {
		ASSERT_UNREACHABLE;
	}
}

template <typename T>
void
test_pmem()
{
	using string_type = nvobj::experimental::basic_inline_string<T>;
	try {
		constexpr size_t string_size = 20;
		string_type(string_size);
		ASSERT_UNREACHABLE;
	} catch (pmem::pool_error &) {
	} catch (...) {
		ASSERT_UNREACHABLE;
	}
}

}

template <typename T>
static void
test(int argc, char *argv[])
{
	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<struct root<T>> pop;

	try {
		pop = nvobj::pool<root<T>>::create(
			std::string(path) + "_" + typeid(T).name(), LAYOUT,
			PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

	test_inline_string<T>(pop);
	test_dram<T>(pop);
	test_pmem<T>();

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] {
		test<char>(argc, argv);
		test<wchar_t>(argc, argv);
		test<uint8_t>(argc, argv);
	});
}
