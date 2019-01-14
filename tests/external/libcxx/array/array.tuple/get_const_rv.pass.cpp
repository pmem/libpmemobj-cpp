//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Copyright 2018, Intel Corporation
//
// Modified to test pmem::obj containers
//

#include "unittest.hpp"

#include <libpmemobj++/experimental/array.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

namespace pmem_exp = pmem::obj::experimental;

using pmem_exp::get;

struct Testcase1 {
	typedef std::unique_ptr<double> T;
	typedef pmem_exp::array<T, 1> C;
	const C c = {{std::unique_ptr<double>(new double(3.5))}};

	void
	run()
	{
		static_assert(
			std::is_same<const T &&,
				     decltype(get<0>(std::move(c)))>::value,
			"");
		static_assert(noexcept(get<0>(std::move(c))), "");
		const T &&t = get<0>(std::move(c));
		UT_ASSERT(*t == 3.5);
	}
};

struct root {
	pmem::obj::persistent_ptr<Testcase1> r1;
};

void
run(pmem::obj::pool<root> &pop)
{
	try {
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->r1 =
				pmem::obj::make_persistent<Testcase1>();
		});

		pmem::obj::transaction::run(pop,
					    [&] { pop.root()->r1->run(); });
	} catch (...) {
		UT_ASSERT(0);
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
		pop = pmem::obj::pool<root>::create(path, "get_cons_rv.pass",
						    PMEMOBJ_MIN_POOL,
						    S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	run(pop);

	return 0;
}
