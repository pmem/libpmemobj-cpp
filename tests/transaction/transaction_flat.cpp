// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "transaction.hpp"
#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/iterator_base.h>

namespace nvobj = pmem::obj;

using huge_object = char[1ULL << 30];

const int ABORT_VAL = 0xABC;

template <typename T>
struct simple_ptr {
	simple_ptr()
	{
		ptr = nvobj::make_persistent<T>();
	}
	~simple_ptr()
	{
		UT_ASSERT(ptr != nullptr);

		try {
			nvobj::delete_persistent<T>(ptr);
		} catch (...) {
			UT_ASSERT(0);
		}
	}

	nvobj::persistent_ptr<T> ptr;
};

template <typename T>
struct simple_ptr_tx {
	simple_ptr_tx()
	{
		auto pop = nvobj::pool_base(pmemobj_pool_by_ptr(this));

		nvobj::transaction::run(
			pop, [&] { ptr = nvobj::make_persistent<T>(); });
	}
	~simple_ptr_tx()
	{
		UT_ASSERT(ptr != nullptr);

		try {
			nvobj::delete_persistent<T>(ptr);
		} catch (...) {
			UT_ASSERT(0);
		}
	}

	nvobj::persistent_ptr<T> ptr;
};

template <typename T>
struct simple_ptr_explicit_abort {
	static nvobj::pool_base *pop;

	simple_ptr_explicit_abort(nvobj::pool_base &pop)
	{
		simple_ptr_explicit_abort::pop = &pop;

		ptr = nvobj::make_persistent<T>();
	}

	~simple_ptr_explicit_abort()
	{
		auto oid = pmemobj_first(pop->handle());
		UT_ASSERT(OID_IS_NULL(oid));
	}

	nvobj::persistent_ptr<T> ptr;
};

template <typename T>
nvobj::pool_base *simple_ptr_explicit_abort<T>::pop = nullptr;

struct C {
	C() : b()
	{
		nvobj::make_persistent<huge_object>();
	}

	simple_ptr<int> b;
};

struct C_tx {
	C_tx() : b()
	{
		nvobj::make_persistent<huge_object>();
	}

	simple_ptr_tx<int> b;
};

struct C_explicit_abort {
	C_explicit_abort(nvobj::pool_base &pop) : b(pop)
	{
		nvobj::transaction::abort(ABORT_VAL);
	}

	simple_ptr_explicit_abort<int> b;
};

struct C_nested {
	C_nested() : b()
	{
		nvobj::make_persistent<huge_object>();
	}

	simple_ptr<simple_ptr<int>> b;
};

struct root {
	nvobj::persistent_ptr<C> c_ptr;
	nvobj::persistent_ptr<C_tx> c_ptr_tx;
	nvobj::persistent_ptr<C_nested> c_nested_ptr;
	nvobj::persistent_ptr<C_explicit_abort> c_explicit_abort_ptr;

	nvobj::persistent_ptr<int> p1;
	nvobj::persistent_ptr<int> p2;
	nvobj::persistent_ptr<int> p3;
};

namespace
{

void
test_dtor_after_tx_abort(nvobj::pool<struct root> &pop)
{
	try {
		nvobj::transaction::run(pop, [&] {
			pop.root()->c_ptr = nvobj::make_persistent<C>();
		});

		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	auto oid = pmemobj_first(pop.handle());
	UT_ASSERT(OID_IS_NULL(oid));

	try {
		nvobj::transaction::run(pop, [&] {
			pop.root()->c_ptr_tx = nvobj::make_persistent<C_tx>();
		});

		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	oid = pmemobj_first(pop.handle());
	UT_ASSERT(OID_IS_NULL(oid));
}

void
test_dtor_after_tx_explicit_abort(nvobj::pool<struct root> &pop)
{
	try {
		nvobj::transaction::run(pop, [&] {
			pop.root()->c_explicit_abort_ptr =
				nvobj::make_persistent<C_explicit_abort>(pop);
		});

		UT_ASSERT(0);
	} catch (pmem::manual_tx_abort &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ABORT_VAL);
	auto oid = pmemobj_first(pop.handle());
	UT_ASSERT(OID_IS_NULL(oid));
}

void
test_nested_dtor_after_tx_abort(nvobj::pool<struct root> &pop)
{
	try {
		nvobj::transaction::run(pop, [&] {
			pop.root()->c_nested_ptr =
				nvobj::make_persistent<C_nested>();
		});

		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	auto oid = pmemobj_first(pop.handle());
	UT_ASSERT(OID_IS_NULL(oid));
}

void
test_ignore_exception(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->p1 = nvobj::make_persistent<int>();
			r->p2 = nvobj::make_persistent<int>();
			r->p3 = nvobj::make_persistent<int>();

			try {
				nvobj::make_persistent<huge_object>();
			} catch (...) {
				/* ignore exception */
			}
		});

		/* p1, p2, p3 are still accessible */
		UT_ASSERTne(pmemobj_pool_by_oid(r->p1.raw()), nullptr);
		UT_ASSERTne(pmemobj_pool_by_oid(r->p2.raw()), nullptr);
		UT_ASSERTne(pmemobj_pool_by_oid(r->p3.raw()), nullptr);
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	/* p1, p2, p3 are still accessible */
	UT_ASSERTne(pmemobj_pool_by_oid(r->p1.raw()), nullptr);
	UT_ASSERTne(pmemobj_pool_by_oid(r->p2.raw()), nullptr);
	UT_ASSERTne(pmemobj_pool_by_oid(r->p3.raw()), nullptr);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		nvobj::delete_persistent<int>(r->p2);
		nvobj::delete_persistent<int>(r->p3);

		r->p1 = nullptr;
		r->p2 = nullptr;
		r->p3 = nullptr;
	});
}

void
test_memory_is_freed_explicit_abort(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->p1 = nvobj::make_persistent<int>();
			r->p2 = nvobj::make_persistent<int>();
			r->p3 = nvobj::make_persistent<int>();

			try {
				nvobj::transaction::abort(-1);
			} catch (...) {
				/* ignore exception */
			}

			UT_ASSERTeq(nvobj::transaction::error(), -1);
		});

		UT_ASSERT(0);
	} catch (pmem::transaction_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(nvobj::transaction::error(), -1);
	auto oid = pmemobj_first(pop.handle());
	UT_ASSERT(OID_IS_NULL(oid));
}

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
					throw std::runtime_error("error");
				});
			} catch (std::runtime_error &) {
				exception_thrown = true;
			} catch (...) {
				UT_ASSERT(0);
			}
			UT_ASSERT(exception_thrown);
			exception_thrown = false;
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(!exception_thrown);
	UT_ASSERT(r->p1 != nullptr);
	UT_ASSERTne(pmemobj_pool_by_oid(r->p1.raw()), nullptr);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		r->p1 = nullptr;
	});
}

void
test_tx_nested_behavior(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	bool exception_thrown = false;

	try {
		nvobj::transaction::run(pop, [&]() {
			UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
				  POBJ_TX_FAILURE_RETURN);

			r->p1 = nvobj::make_persistent<int>();
			try {
				nvobj::basic_transaction::run(pop, [&]() {
					throw std::runtime_error("error");
				});
			} catch (std::runtime_error &) {
				UT_ASSERT(pmemobj_tx_stage() ==
					  TX_STAGE_ONABORT);
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

	UT_ASSERTne(nvobj::transaction::error(), 0);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(r->p1 == nullptr);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		r->p1 = nullptr;
	});
}

template <typename OuterFlatTx, typename InnerBasicTx>
void
test_tx_nested_behavior_scope(nvobj::pool<struct root> &pop)
{
	bool exception_thrown = false;

	auto r = pop.root();

	try {
		counter = 0;
		OuterFlatTx to(pop);
		UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
			  POBJ_TX_FAILURE_RETURN);

		r->p1 = nvobj::make_persistent<int>();
		try {
			InnerBasicTx to_nested(pop);
			counter = 1;
			throw std::runtime_error("error");
			UT_ASSERT(0);
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

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);

	if (std::is_same<OuterFlatTx,
			 nvobj::flat_transaction::automatic>::value)
		UT_ASSERT(exception_thrown);
	else
		UT_ASSERT(!exception_thrown);
	UT_ASSERT(r->p1 == nullptr);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		r->p1 = nullptr;
	});
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
			counter = 1;
			throw std::runtime_error("error");
		} catch (std::runtime_error &) {
			exception_thrown = true;
		} catch (...) {
			UT_ASSERT(0);
		}

		UT_ASSERT(exception_thrown);
		exception_thrown = false;

		if (std::is_same<T, nvobj::flat_transaction::manual>::value)
			nvobj::transaction::commit();

	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(!exception_thrown);
	UT_ASSERT(r->p1 != nullptr);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		r->p1 = nullptr;
	});
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

	/* the transaction won't be aborted since exception was handled */
	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(!exception_thrown);
	UT_ASSERT(r->p1 != nullptr);

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		r->p1 = nullptr;
	});
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

		/* For flat transactions, it's not necessary to call commit for
		 * inner transactions. */
		UT_ASSERT(r->p1 != nullptr);
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

	test_ignore_exception(pop);

	test_tx_throw_no_abort(pop);

	test_memory_is_freed_explicit_abort(pop);

	test_dtor_after_tx_explicit_abort(pop);

	test_dtor_after_tx_abort(pop);
	test_nested_dtor_after_tx_abort(pop);

	test_tx_nested_behavior(pop);

	test_tx_nested_behavior_scope<nvobj::transaction::manual,
				      nvobj::basic_transaction::manual>(pop);
	test_tx_nested_behavior_scope<nvobj::transaction::automatic,
				      nvobj::basic_transaction::automatic>(pop);

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
