// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

#include <map>

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

template <typename T>
class Basic_String_Container {
public:
	static std::map<std::string, std::basic_string<T>> string_map;
};

template <typename T>
typename std::map<std::string, std::basic_string<T>>
	Basic_String_Container<T>::string_map;

template <typename CharT>
struct Object {
	template <typename... Args>
	Object(int d, Args &&... args) : data(d), s(std::forward<Args>(args)...)
	{
	}

	Object(const Object &o) = default;

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
typename nvobj::basic_string_view<T>
convert_to_basic_string_view(std::string str)
{
	Basic_String_Container<T>::string_map[str] =
		std::basic_string<T>(str.begin(), str.end());
	nvobj::basic_string_view<T> bsv(
		Basic_String_Container<T>::string_map[str].data(),
		Basic_String_Container<T>::string_map[str].length());
	return bsv;
}

template <typename T>
void
test_inline_string(nvobj::pool<struct root<T>> &pop)
{
	auto r = pop.root();

	std::string test_string1("abcd");
	std::string test_string2("xxxxxxx");
	std::string test_string3("abcdefgh");

	nvobj::basic_string_view<T> bsv_test_string1(
		convert_to_basic_string_view<T>(test_string1));
	nvobj::basic_string_view<T> bsv_test_string2(
		convert_to_basic_string_view<T>(test_string2));
	nvobj::basic_string_view<T> test_view(
		convert_to_basic_string_view<T>(test_string3));

	const nvobjex::inline_string::size_type req_capacity = 100;

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::standard_alloc_policy<void> allocator;
			r->o1 = static_cast<nvobj::persistent_ptr<Object<T>>>(
				allocator.allocate(sizeof(Object<T>) +
						   req_capacity));
			r->o2 = static_cast<nvobj::persistent_ptr<Object<T>>>(
				allocator.allocate(sizeof(Object<T>) +
						   req_capacity));
			r->o3 = static_cast<nvobj::persistent_ptr<Object<T>>>(
				allocator.allocate(sizeof(Object<T>) +
						   req_capacity));

			new (r->o1.get()) Object<T>(1, req_capacity);
			new (r->o2.get()) Object<T>(2, req_capacity);
			new (r->o3.get()) Object<T>(
				3,
				convert_to_basic_string_view<T>(
					std::string(req_capacity, 'a')));

			r->o1->s.assign(bsv_test_string1);
			r->o2->s.assign(bsv_test_string2);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(r->o1->data, 1);
	UT_ASSERTeq(r->o2->data, 2);
	UT_ASSERTeq(r->o3->data, 3);

	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(
			  bsv_test_string1) == 0);
	UT_ASSERT(r->o1->s.size() == 4);
	UT_ASSERT(memcmp(r->o1->s.data(), bsv_test_string1.data(), 4) == 0);

	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).data()[4] == '\0');
	UT_ASSERT(nvobj::basic_string_view<T>(r->o2->s).data()[7] == '\0');
	UT_ASSERT(nvobj::basic_string_view<T>(r->o2->s).compare(
			  bsv_test_string2) == 0);

	UT_ASSERT(r->o3->s.capacity() == r->o3->s.size());
	UT_ASSERT(nvobj::basic_string_view<T>(r->o3->s).compare(
			  convert_to_basic_string_view<T>(
				  std::string(req_capacity, 'a'))) == 0);

	try {
		nvobj::transaction::run(pop, [&] {
			*(r->o1) = *(r->o2);

			UT_ASSERTeq(r->o1->data, 2);
			UT_ASSERTeq(r->o2->data, 2);

			UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(
					  bsv_test_string2) == 0);
			UT_ASSERT(nvobj::basic_string_view<T>(r->o2->s).compare(
					  bsv_test_string2) == 0);

			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERTeq(r->o1->data, 1);
	UT_ASSERTeq(r->o2->data, 2);

	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(
			  bsv_test_string1) == 0);
	UT_ASSERT(nvobj::basic_string_view<T>(r->o2->s).compare(
			  bsv_test_string2) == 0);

	r->o1->s = test_view;
	UT_ASSERT(test_view.compare(r->o1->s) == 0);

	nvobj::basic_string_view<T> new_view = r->o1->s;
	UT_ASSERT(new_view.compare(test_view) == 0);

	try {
		std::string str(r->o1->s.capacity() + 5, 'x');

		nvobj::basic_string_view<T> v1 =
			convert_to_basic_string_view<T>(str);
		nvobj::transaction::run(pop, [&] { r->o1->s = v1; });
	} catch (std::out_of_range &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	r->o1->s.assign(convert_to_basic_string_view<T>(""));

	try {
		nvobj::transaction::run(pop, [&] {
			r->o1->s.assign(
				convert_to_basic_string_view<T>("aaaa"));

			r->o1->s = r->o1->s;
			UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(
					  convert_to_basic_string_view<T>(
						  "aaaa")) == 0);

			nvobj::transaction::abort(0);
		});
	} catch (pmem::manual_tx_abort &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).compare(
			  convert_to_basic_string_view<T>("")) == 0);
	UT_ASSERT(nvobj::basic_string_view<T>(r->o1->s).data()[0] == '\0');

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<Object<T>>(r->o1);
		nvobj::delete_persistent<Object<T>>(r->o2);
	});
}

template <typename T>
void
test_ctor_exception_nopmem(nvobj::pool<struct root<T>> &pop)
{
	try {
		Object<T> o(1, convert_to_basic_string_view<T>("example"));
	} catch (pmem::pool_error &) {
	} catch (...) {
		UT_ASSERT(0);
	}

	auto r = pop.root();

	const auto req_capacity = 100;

	nvobj::transaction::run(pop, [&] {
		nvobj::standard_alloc_policy<void> allocator;
		r->o1 = static_cast<nvobj::persistent_ptr<Object<T>>>(
			allocator.allocate(sizeof(Object<T>) + req_capacity));

		new (r->o1.get())
			Object<T>(1, convert_to_basic_string_view<T>("abcd"));

		try {
			Object<T> o(*r->o1);
		} catch (pmem::pool_error &) {
		} catch (...) {
			UT_ASSERT(0);
		}
	});
}

template <typename T>
void
test_ctor_exception(void)
{
	try {
		int capacity = 10;
		nvobjex::basic_inline_string<T>(
			static_cast<nvobjex::inline_string::size_type>(
				capacity));
	} catch (pmem::pool_error &) {
	} catch (...) {
		UT_ASSERT(0);
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
	test_ctor_exception_nopmem<T>(pop);
	test_ctor_exception<T>();
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
