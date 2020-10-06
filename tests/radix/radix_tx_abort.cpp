// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "radix.hpp"

namespace
{
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
		UT_ASSERT(it.first->key() == key<Container>(0));
		UT_ASSERT(it.first->value() == value<Container>(0));

		UT_ASSERTeq(ptr->size(), 1);
	});

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		for (unsigned i = 0; i < 1024; i++) {
			auto it = ptr->try_emplace(key<Container>(i),
						   value<Container>(i));
			UT_ASSERT(it.second);
			UT_ASSERT(it.first->key() == key<Container>(i));
			UT_ASSERT(it.first->value() == value<Container>(i));
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
test_try_emplace(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](unsigned v) {
		return value<Container>(v, ValueRepeats);
	};

	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<Container>(); });

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		auto it = ptr->try_emplace(key<Container>(0), value_f(0));
		UT_ASSERT(it.second);
		UT_ASSERT(it.first->key() == key<Container>(0));
		UT_ASSERT(it.first->value() == value_f(0));

		UT_ASSERTeq(ptr->size(), 1);
	});

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		for (unsigned i = 0; i < 1024; i++) {
			auto it = ptr->try_emplace(key<Container>(i),
						   value<Container>(i));
			UT_ASSERT(it.second);
			UT_ASSERT(it.first->key() == key<Container>(i));
			UT_ASSERT(it.first->value() == value_f(i));
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
test_insert_or_assign(nvobj::pool<root> &pop,
		      nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](unsigned v) {
		return value<Container>(v, ValueRepeats);
	};

	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<Container>(); });

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		auto it = ptr->insert_or_assign(key<Container>(0),
						value<Container>(0));
		UT_ASSERT(it.second);
		UT_ASSERT(it.first->key() == key<Container>(0));
		UT_ASSERT(it.first->value() == value<Container>(0));

		UT_ASSERTeq(ptr->size(), 1);
	});

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		for (unsigned i = 0; i < 1024; i++) {
			auto it = ptr->insert_or_assign(key<Container>(i),
							value<Container>(i));
			UT_ASSERT(it.second);
			UT_ASSERT(it.first->key() == key<Container>(i));
			UT_ASSERT(it.first->value() == value<Container>(i));
		}

		UT_ASSERTeq(ptr->size(), 1024);
	});

	UT_ASSERTeq(ptr->size(), 0);

	for (unsigned i = 0; i < 10; i++)
		ptr->insert_or_assign(key<Container>(i), value<Container>(i));

	verify_elements(ptr, 10, key<Container>, value_f);

	assert_tx_abort(pop, [&] {
		for (unsigned i = 0; i < 10; i++) {
			auto it = ptr->insert_or_assign(
				key<Container>(i), value<Container>(i + 1));
			UT_ASSERT(!it.second);
			UT_ASSERT(it.first->key() == key<Container>(i));
			UT_ASSERT(it.first->value() == value<Container>(i + 1));
		}

		UT_ASSERTeq(ptr->size(), 10);
	});

	verify_elements(ptr, 10, key<Container>, value_f);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_insert(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	using V = typename Container::value_type;

	auto value_f = [](unsigned v) {
		return value<Container>(v, ValueRepeats);
	};

	nvobj::transaction::run(
		pop, [&] { ptr = nvobj::make_persistent<Container>(); });

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		V v1(key<Container>(0), value_f(0));
		auto it = ptr->insert(v1);
		UT_ASSERT(it.second);
		UT_ASSERT(it.first->key() == key<Container>(0));
		UT_ASSERT(it.first->value() == value_f(0));

		UT_ASSERTeq(ptr->size(), 1);
	});

	UT_ASSERTeq(ptr->size(), 0);

	assert_tx_abort(pop, [&] {
		for (unsigned i = 0; i < 1024; i++) {
			V v1(key<Container>(i), value_f(i));
			auto it = ptr->insert(v1);
			UT_ASSERT(it.second);
			UT_ASSERT(it.first->key() == key<Container>(i));
			UT_ASSERT(it.first->value() == value_f(i));
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
	auto value_f = [](unsigned v) {
		return value<Container>(v, ValueRepeats);
	};

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		for (unsigned i = 0; i < 10; i++)
			ptr->try_emplace(key<Container>(i), value_f(i));
	});

	UT_ASSERTeq(ptr->size(), 10);

	const auto test_key = 3;
	const auto test_value = 99;

	typename Container::iterator it;

	assert_tx_abort(pop, [&] {
		it = ptr->find(key<Container>(test_key));
		it.assign_val(value_f(test_value));

		UT_ASSERT(it->value() == value_f(test_value));
		UT_ASSERT(ptr->find(key<Container>(test_key))->value() ==
			  value_f(test_value));

		++it;
		UT_ASSERT(it->key() == key<Container>(test_key + 1));
		UT_ASSERT(it->value() == value_f(test_key + 1));

		--it;
		--it;
		UT_ASSERT(it->key() == key<Container>(test_key - 1));
		UT_ASSERT(it->value() == value_f(test_key - 1));

		++it;
	});

	verify_elements(ptr, 10, key<Container>, value_f);

	/* Iterators and references for inline_string are not
	 * stable. */
	if (pmem::detail::is_inline_string<
		    typename Container::mapped_type>::value) {
		it = ptr->find(key<Container>(test_key));
	}

	UT_ASSERT(it->value() == value_f(test_key));

	++it;
	UT_ASSERT(it->key() == key<Container>(test_key + 1));
	UT_ASSERT(it->value() == value_f(test_key + 1));

	--it;
	--it;
	UT_ASSERT(it->key() == key<Container>(test_key - 1));
	UT_ASSERT(it->value() == value_f(test_key - 1));

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_assign_internal_leaf(nvobj::pool<root> &pop,
			  nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](unsigned v) {
		return value<Container>(v, ValueRepeats);
	};

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
		it.assign_val(value_f(new_value));

		UT_ASSERT(it->value() == value_f(new_value));
		UT_ASSERT(ptr->find("")->value() == value_f(new_value));
	});

	/* Iterators and references for inline_string are not
	 * stable. */
	if (pmem::detail::is_inline_string<
		    typename Container::mapped_type>::value) {
		it = ptr->find("");
	}

	UT_ASSERTeq(ptr->size(), 11);
	UT_ASSERT(it->value() == value_f(test_value));
	UT_ASSERT(ptr->find("")->value() == value_f(test_value));

	assert_tx_abort(pop, [&] {
		it = ptr->find("aaa");
		it.assign_val(value_f(new_value));

		UT_ASSERT(it->value() == value_f(new_value));
		UT_ASSERT(ptr->find("aaa")->value() == value_f(new_value));
	});

	/* Iterators and references for inline_string are not
	 * stable. */
	if (pmem::detail::is_inline_string<
		    typename Container::mapped_type>::value) {
		it = ptr->find("aaa");
	}

	UT_ASSERTeq(ptr->size(), 11);
	UT_ASSERT(it->value() == value_f(test_value));
	UT_ASSERT(ptr->find("aaa")->value() == value_f(test_value));

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_assign_root(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](unsigned v) {
		return value<Container>(v, ValueRepeats);
	};

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		ptr->try_emplace(key<Container>(0), value_f(0));
	});

	UT_ASSERTeq(ptr->size(), 1);

	typename Container::iterator it;

	assert_tx_abort(pop, [&] {
		it = ptr->find(key<Container>(0));
		it.assign_val(value_f(1));

		UT_ASSERT(it->value() == value_f(1));
		UT_ASSERT(ptr->find(key<Container>(0))->value() == value_f(1));
	});

	verify_elements(ptr, 1, key<Container>, value_f);

	/* Iterators and references for inline_string are not
	 * stable. */
	if (pmem::detail::is_inline_string<
		    typename Container::mapped_type>::value) {
		it = ptr->find(key<Container>(0));
	}

	UT_ASSERT(it->value() == value_f(0));
	UT_ASSERT(ptr->find(key<Container>(0))->value() == value_f(0));

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_erase(nvobj::pool<root> &pop, nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](unsigned v) {
		return value<Container>(v, ValueRepeats);
	};
	const unsigned int num_elements = 1024;

	nvobj::transaction::run(pop, [&] {
		ptr = nvobj::make_persistent<Container>();
		for (unsigned i = 0; i < num_elements; i++) {
			auto ret =
				ptr->try_emplace(key<Container>(i), value_f(i));
			UT_ASSERT(ret.second);
		}
	});

	verify_elements(ptr, num_elements, key<Container>, value_f);

	assert_tx_abort(pop, [&] {
		for (unsigned i = 0; i < num_elements; i++) {
			UT_ASSERTeq(ptr->size(),
				    static_cast<size_t>(num_elements - i));
			UT_ASSERTeq(ptr->erase(key<Container>(i)), 1);
		}
	});

	verify_elements(ptr, num_elements, key<Container>, value_f);

	assert_tx_abort(pop, [&] {
		for (unsigned i = num_elements; i > 0; i--) {
			UT_ASSERTeq(ptr->size(), static_cast<size_t>(i));
			UT_ASSERTeq(ptr->erase(key<Container>(i - 1)), 1);
		}
	});

	verify_elements(ptr, num_elements, key<Container>, value_f);

	nvobj::transaction::run(
		pop, [&] { nvobj::delete_persistent<Container>(ptr); });

	UT_ASSERT(OID_IS_NULL(pmemobj_first(pop.handle())));
}

template <typename Container, int ValueRepeats>
void
test_erase_internal(nvobj::pool<root> &pop,
		    nvobj::persistent_ptr<Container> &ptr)
{
	auto value_f = [](unsigned v) {
		return value<Container>(v, ValueRepeats);
	};

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
		pop = nvobj::pool<struct root>::create(path, "radix",
						       10 * PMEMOBJ_MIN_POOL,
						       S_IWUSR | S_IRUSR);
	} catch (pmem::pool_error &pe) {
		UT_FATAL("!pool::create: %s %s", pe.what(), path);
	}

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
	test_insert_or_assign<container_string, 1>(pop, pop.root()->radix_str);
	test_try_emplace<container_string, 1>(pop, pop.root()->radix_str);

	test_emplace(pop, pop.root()->radix_int);
	test_assign<container_int, 1>(pop, pop.root()->radix_int);
	test_assign_root<container_int, 1>(pop, pop.root()->radix_int);
	test_erase<container_int, 1024>(pop, pop.root()->radix_int);
	test_assign_internal_leaf<container_int, 1>(pop, pop.root()->radix_int);
	test_erase_internal<container_int, 1024>(pop, pop.root()->radix_int);
	test_insert_or_assign<container_int, 1>(pop, pop.root()->radix_int);
	test_try_emplace<container_int, 1>(pop, pop.root()->radix_int);

	test_emplace(pop, pop.root()->radix_int_int);
	test_assign<container_int_int, 1>(pop, pop.root()->radix_int_int);
	test_assign_root<container_int_int, 1>(pop, pop.root()->radix_int_int);
	test_erase<container_int_int, 1>(pop, pop.root()->radix_int_int);
	test_insert<container_int_int, 1>(pop, pop.root()->radix_int_int);
	test_insert_or_assign<container_int_int, 1>(pop,
						    pop.root()->radix_int_int);
	test_try_emplace<container_int_int, 1>(pop, pop.root()->radix_int_int);

	test_emplace(pop, pop.root()->radix_int_str);
	test_assign<container_int_string, 1>(pop, pop.root()->radix_int_str);
	test_assign<container_int_string, 1024>(pop, pop.root()->radix_int_str);
	test_assign_root<container_int_string, 1>(pop,
						  pop.root()->radix_int_str);
	test_assign_root<container_int_string, 1024>(pop,
						     pop.root()->radix_int_str);
	test_erase<container_int_string, 1024>(pop, pop.root()->radix_int_str);
	test_insert_or_assign<container_int_string, 1>(
		pop, pop.root()->radix_int_str);
	test_try_emplace<container_int_string, 1>(pop,
						  pop.root()->radix_int_str);

	test_emplace(pop, pop.root()->radix_inline_s_u8t);
	test_assign<container_inline_s_u8t, 1>(pop,
					       pop.root()->radix_inline_s_u8t);
	test_assign<container_inline_s_u8t, 1024>(
		pop, pop.root()->radix_inline_s_u8t);
	test_assign_root<container_inline_s_u8t, 1>(
		pop, pop.root()->radix_inline_s_u8t);
	test_assign_root<container_inline_s_u8t, 1024>(
		pop, pop.root()->radix_inline_s_u8t);
	test_erase<container_inline_s_u8t, 1024>(
		pop, pop.root()->radix_inline_s_u8t);
	test_insert_or_assign<container_inline_s_u8t, 1>(
		pop, pop.root()->radix_inline_s_u8t);
	test_try_emplace<container_inline_s_u8t, 1>(
		pop, pop.root()->radix_inline_s_u8t);

	test_emplace(pop, pop.root()->radix_inline_s_wchart);
	test_assign<container_inline_s_wchart, 1>(
		pop, pop.root()->radix_inline_s_wchart);
	test_assign<container_inline_s_wchart, 1024>(
		pop, pop.root()->radix_inline_s_wchart);
	test_assign_root<container_inline_s_wchart, 1>(
		pop, pop.root()->radix_inline_s_wchart);
	test_assign_root<container_inline_s_wchart, 1024>(
		pop, pop.root()->radix_inline_s_wchart);
	test_erase<container_inline_s_wchart, 1024>(
		pop, pop.root()->radix_inline_s_wchart);
	test_insert_or_assign<container_inline_s_wchart, 1>(
		pop, pop.root()->radix_inline_s_wchart);
	test_try_emplace<container_inline_s_wchart, 1>(
		pop, pop.root()->radix_inline_s_wchart);

	test_emplace(pop, pop.root()->radix_inline_s_wchart_wchart);
	test_assign<container_inline_s_wchart_wchart, 1>(
		pop, pop.root()->radix_inline_s_wchart_wchart);
	test_assign<container_inline_s_wchart_wchart, 1024>(
		pop, pop.root()->radix_inline_s_wchart_wchart);
	test_assign_root<container_inline_s_wchart_wchart, 1>(
		pop, pop.root()->radix_inline_s_wchart_wchart);
	test_assign_root<container_inline_s_wchart_wchart, 1024>(
		pop, pop.root()->radix_inline_s_wchart_wchart);
	test_erase<container_inline_s_wchart_wchart, 1024>(
		pop, pop.root()->radix_inline_s_wchart_wchart);
	test_insert_or_assign<container_inline_s_wchart_wchart, 1>(
		pop, pop.root()->radix_inline_s_wchart_wchart);
	test_try_emplace<container_inline_s_wchart_wchart, 1>(
		pop, pop.root()->radix_inline_s_wchart_wchart);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
