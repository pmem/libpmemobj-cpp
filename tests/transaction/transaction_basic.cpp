// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "transaction.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

namespace nvobj = pmem::obj;

using huge_object = char[1ULL << 30];

struct root {
	nvobj::persistent_ptr<int> p1;
	nvobj::persistent_ptr<int> p2;
};

namespace
{
void
test_tx_throw_no_abort(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	bool exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&]() {
			r->p1 = nvobj::make_persistent<int>();
			try {
				nvobj::transaction::run(pop, [&]() {
					r->p2 = nvobj::make_persistent<int>();
					throw std::runtime_error("error");
				});
			} catch (std::runtime_error &) {
				UT_ASSERT(pmemobj_tx_stage() ==
					  TX_STAGE_ONABORT);

				UT_ASSERT(r->p1 == nullptr);
				UT_ASSERT(r->p2 == nullptr);

				exception_thrown = true;
			} catch (...) {
				UT_ASSERT(0);
			}
			UT_ASSERT(exception_thrown);
			exception_thrown = false;
		});
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->p1 == nullptr);
	UT_ASSERT(r->p2 == nullptr);
}

void
test_tx_nested_behavior(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	bool exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&]() {
			UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
				  POBJ_TX_FAILURE_ABORT);

			r->p1 = nvobj::make_persistent<int>();
			try {
				nvobj::flat_transaction::run(pop, [&]() {
					UT_ASSERT(
						pmemobj_tx_get_failure_behavior() ==
						POBJ_TX_FAILURE_RETURN);

					r->p2 = nvobj::make_persistent<int>();
					throw std::runtime_error("error");
				});
			} catch (std::runtime_error &) {
				UT_ASSERT(pmemobj_tx_stage() == TX_STAGE_WORK);

				UT_ASSERT(r->p1 != nullptr);
				UT_ASSERT(r->p2 != nullptr);

				exception_thrown = true;
			} catch (...) {
				UT_ASSERT(0);
			}
			UT_ASSERT(exception_thrown);
			exception_thrown = false;
		});
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(!exception_thrown);
	UT_ASSERT(r->p1 != nullptr);
	UT_ASSERT(r->p2 != nullptr);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		nvobj::delete_persistent<int>(r->p2);

		r->p1 = nullptr;
		r->p2 = nullptr;
	});

	exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&]() {
			UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
				  POBJ_TX_FAILURE_ABORT);

			r->p1 = nvobj::make_persistent<int>();

			nvobj::flat_transaction::run(pop, [&]() {
				UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
					  POBJ_TX_FAILURE_RETURN);

				r->p2 = nvobj::make_persistent<int>();
				throw std::runtime_error("error");
			});
		});
	} catch (std::runtime_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->p1 == nullptr);
	UT_ASSERT(r->p2 == nullptr);
}

template <typename OuterBasicTx, typename InnerFlatTx>
void
test_tx_nested_behavior_scope(nvobj::pool<struct root> &pop)
{
	bool exception_thrown = false;

	auto r = pop.root();

	try {
		counter = 0;
		OuterBasicTx to(pop);
		UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
			  POBJ_TX_FAILURE_ABORT);

		r->p1 = nvobj::make_persistent<int>();
		try {
			InnerFlatTx to_nested(pop);
			UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
				  POBJ_TX_FAILURE_RETURN);

			r->p2 = nvobj::make_persistent<int>();
			counter = 1;
			throw std::runtime_error("error");
		} catch (std::runtime_error &) {
			UT_ASSERT(pmemobj_tx_stage() == TX_STAGE_WORK);

			UT_ASSERT(r->p1 != nullptr);
			UT_ASSERT(r->p2 != nullptr);

			exception_thrown = true;
		} catch (...) {
			UT_ASSERT(0);
		}

		if (std::is_same<OuterBasicTx,
				 nvobj::basic_transaction::manual>::value)
			nvobj::transaction::commit();

		UT_ASSERT(exception_thrown);
		exception_thrown = false;
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(!exception_thrown);
	UT_ASSERT(r->p1 != nullptr);
	UT_ASSERT(r->p2 != nullptr);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		nvobj::delete_persistent<int>(r->p2);

		r->p1 = nullptr;
		r->p2 = nullptr;
	});

	exception_thrown = false;

	try {
		counter = 0;
		OuterBasicTx to(pop);
		UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
			  POBJ_TX_FAILURE_ABORT);

		r->p1 = nvobj::make_persistent<int>();
		{
			InnerFlatTx to_nested(pop);
			UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
				  POBJ_TX_FAILURE_RETURN);

			r->p2 = nvobj::make_persistent<int>();
			counter = 1;
			throw std::runtime_error("error");
		}
	} catch (std::runtime_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->p1 == nullptr);
	UT_ASSERT(r->p2 == nullptr);
}

template <typename T>
void
test_tx_throw_no_abort_scope(nvobj::pool<root> &pop)
{
	bool exception_thrown = false;

	auto r = pop.root();

	try {
		counter = 0;
		T to(pop);
		r->p1 = nvobj::make_persistent<int>();
		try {
			T to_nested(pop);
			r->p2 = nvobj::make_persistent<int>();
			counter = 1;
			throw std::runtime_error("error");
		} catch (std::runtime_error &) {
			UT_ASSERT(pmemobj_tx_stage() == TX_STAGE_ONABORT);

			UT_ASSERT(r->p1 == nullptr);
			UT_ASSERT(r->p2 == nullptr);

			exception_thrown = true;
		} catch (...) {
			UT_ASSERT(0);
		}
		UT_ASSERT(exception_thrown);
		exception_thrown = false;
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	/* the transaction will be aborted silently */
	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	if (std::is_same<T, nvobj::transaction::automatic>::value)
		UT_ASSERT(exception_thrown);
	else
		UT_ASSERT(!exception_thrown);
	UT_ASSERT(r->p1 == nullptr);
	UT_ASSERT(r->p2 == nullptr);
}

void
test_tx_automatic_destructor_throw(nvobj::pool<root> &pop)
{
	bool exception_thrown = false;

	auto r = pop.root();

	try {
		counter = 0;
		nvobj::transaction::automatic to(pop);
		r->p1 = nvobj::make_persistent<int>();
		try {
			nvobj::transaction::automatic to_nested(pop);
			counter = 1;
			throw std::runtime_error("error");
		} catch (std::runtime_error &) {
			UT_ASSERT(pmemobj_tx_stage() == TX_STAGE_ONABORT);
			exception_thrown = true;
		} catch (...) {
			UT_ASSERT(0);
		}
		UT_ASSERT(exception_thrown);
		exception_thrown = false;
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	/* the transaction will be aborted silently */
	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->p1 == nullptr);
}

/*
 * test_tx_manual_no_commit -- test manual transaction with no commit
 */
void
test_tx_manual_no_commit(nvobj::pool<root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::manual tx(pop);
		r->p1 = nvobj::make_persistent<int>();
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->p1 == nullptr);

	try {
		nvobj::transaction::manual tx1(pop);
		{
			nvobj::transaction::manual tx2(pop);
			r->p1 = nvobj::make_persistent<int>();
		}

		UT_ASSERT(r->p1 == nullptr);
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(r->p1 == nullptr);
}

void
test(int argc, char *argv[])
{
	if (argc < 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	auto path = argv[1];
	auto pop =
		nvobj::pool<root>::create(path, "transaction_noabort",
					  PMEMOBJ_MIN_POOL, S_IWUSR | S_IRUSR);

	test_tx_throw_no_abort(pop);

	test_tx_nested_behavior(pop);

	test_tx_nested_behavior_scope<nvobj::transaction::manual,
				      nvobj::flat_transaction::manual>(pop);
	test_tx_nested_behavior_scope<nvobj::transaction::automatic,
				      nvobj::flat_transaction::automatic>(pop);

	test_tx_throw_no_abort_scope<nvobj::transaction::manual>(pop);
	test_tx_throw_no_abort_scope<nvobj::transaction::automatic>(pop);

	test_tx_automatic_destructor_throw(pop);

	test_tx_manual_no_commit(pop);

	pop.close();
}
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
