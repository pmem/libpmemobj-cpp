//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2019, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem_exp = pmem::obj::experimental;
namespace nvobj = pmem::obj;

using string_type = pmem_exp::string;

struct root {
	nvobj::persistent_ptr<string_type> s1;
	nvobj::persistent_ptr<string_type> s2;
};

template <class S>
void
test(S &s0, pmem::obj::pool<root> &pop)
{
	using T = typename string_type::traits_type;
	auto r = pop.root();

	nvobj::transaction::run(
		pop, [&] { r->s1 = nvobj::make_persistent<string_type>(s0); });

	nvobj::transaction::run(pop, [&] {
		r->s2 = nvobj::make_persistent<string_type>(std::move(s0));
	});

	UT_ASSERT(s0.size() == 0);
	UT_ASSERT(*r->s2 == *r->s1);

	UT_ASSERT(r->s1->size() == r->s2->size());
	UT_ASSERT(T::compare(r->s2->c_str(), r->s1->c_str(), r->s1->size()) ==
		  0);
	UT_ASSERT(r->s2->capacity() >= r->s2->size());

	nvobj::transaction::run(pop, [&] {
		UT_ASSERT(r->s2->c_str() == r->s2->data());
		UT_ASSERT(r->s2->c_str() == r->s2->cdata());
		UT_ASSERT(r->s2->c_str() ==
			  static_cast<const string_type &>(*r->s2).data());
	});

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<string_type>(r->s1);
		nvobj::delete_persistent<string_type>(r->s2);
	});
}

void
run(pmem::obj::pool<root> &pop)
{
	try {
		nvobj::persistent_ptr<string_type> s1, s2, s3, s4;

		nvobj::transaction::run(pop, [&] {
			s1 = nvobj::make_persistent<string_type>();
		});

		nvobj::transaction::run(pop, [&] {
			s2 = nvobj::make_persistent<string_type>("1");
		});

		nvobj::transaction::run(pop, [&] {
			s3 = nvobj::make_persistent<string_type>("1234567890");
		});

		nvobj::transaction::run(pop, [&] {
			s4 = nvobj::make_persistent<string_type>(
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890123456789012345678901234567890"
				"1234567890");
		});

		test(*s1, pop);
		test(*s2, pop);
		test(*s3, pop);
		test(*s4, pop);

		nvobj::transaction::run(pop, [&] {
			nvobj::delete_persistent<string_type>(s1);
			nvobj::delete_persistent<string_type>(s2);
			nvobj::delete_persistent<string_type>(s3);
			nvobj::delete_persistent<string_type>(s4);
		});

	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::create(path, "string_test",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	pop.close();

	return 0;
}
