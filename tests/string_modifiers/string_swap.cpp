// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/make_persistent.hpp>

#include <ctime>
#include <string>

namespace nvobj = pmem::obj;

using S = pmem::obj::string;

struct root {
	nvobj::persistent_ptr<S> str1, str2;
};

/* generate std::string with: min <= size < (min + length) */
std::string
generate_string(size_t min, size_t length)
{
	auto get_char = []() {
		const char charset[] = "abcdefghijklmnopqrstuvwxyz"
				       "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[(size_t)rand() % max_index];
	};
	size_t size = (size_t)rand() % length + min;
	std::string result(size, 0);
	std::generate_n(result.begin(), size, get_char);
	return result;
}

void
test_swap(nvobj::pool<root> &pop, std::string &s1, std::string &s2)
{
	nvobj::transaction::run(pop, [&] {
		pop.root()->str1 = nvobj::make_persistent<S>(s1);
		pop.root()->str2 = nvobj::make_persistent<S>(s2);
	});

	auto &left = *pop.root()->str1;
	auto &right = *pop.root()->str2;

	left.swap(right);

	UT_ASSERT(left == s2);
	UT_ASSERT(left.size() == s2.size());
	UT_ASSERT(right == s1);
	UT_ASSERT(right.size() == s1.size());

	pmem::obj::swap(right, left);

	UT_ASSERT(left == s1);
	UT_ASSERT(left.size() == s1.size());
	UT_ASSERT(right == s2);
	UT_ASSERT(right.size() == s2.size());

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<S>(pop.root()->str1);
		nvobj::delete_persistent<S>(pop.root()->str2);
	});
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "StringTest", PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	auto seed = unsigned(std::time(0));
	printf("%u\n", seed);
	std::srand(seed);

	try {
		std::string s1(generate_string(1, S::sso_capacity - 1));
		std::string s2(generate_string(1, S::sso_capacity - 1));
		// sso - sso
		test_swap(pop, s1, s2);

		s1 = generate_string(S::sso_capacity + 1, S::sso_capacity);
		// non_sso - sso
		test_swap(pop, s1, s2);

		s2 = generate_string(S::sso_capacity + 1, S::sso_capacity);
		// non_sso - non_sso
		test_swap(pop, s1, s2);

		s1 = generate_string(1, S::sso_capacity - 1);
		// sso - non_sso
		test_swap(pop, s1, s2);
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
