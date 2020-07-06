// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#ifndef TRANSACTION_HELPERS_HPP
#define TRANSACTION_HELPERS_HPP

#include "unittest.hpp"

#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

void
assert_tx_abort(pmem::obj::pool<struct root> &pop, std::function<void(void)> f)
{
	bool exception_thrown = false;
	try {
		pmem::obj::transaction::run(pop, [&] {
			f();
			pmem::obj::transaction::abort(EINVAL);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
	UT_ASSERT(exception_thrown);
}

#endif /* TRANSACTION_HELPERS_HPP */
