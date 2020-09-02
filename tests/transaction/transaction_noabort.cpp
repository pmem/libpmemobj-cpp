// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>

#include <libpmemobj/iterator_base.h>

namespace
{
int counter = 0;
}

/*
 * XXX The Microsoft compiler does not follow the ISO SD-6: SG10 Feature
 * Test Recommendations. "_MSC_VER" is a workaround.
 */
#if _MSC_VER < 1900
#ifndef __cpp_lib_uncaught_exceptions
#define __cpp_lib_uncaught_exceptions 201411
namespace std
{

int
uncaught_exceptions() noexcept
{
	return ::counter;
}

} /* namespace std */
#endif /* __cpp_lib_uncaught_exceptions */
#endif /* _MSC_VER */

#include <libpmemobj++/transaction.hpp>

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

		nvobj::delete_persistent<T>(ptr);
	}

	nvobj::persistent_ptr<T> ptr;
};

struct C {
	C() : b()
	{
		nvobj::make_persistent<huge_object>();
	}

	simple_ptr<int> b;
};

struct C_explicit_abort {
	C_explicit_abort() : b()
	{
		nvobj::transaction::abort(ABORT_VAL);
	}

	simple_ptr<int> b;
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
	nvobj::persistent_ptr<C_nested> c_nested_ptr;
	nvobj::persistent_ptr<C_explicit_abort> c_explicit_abort_ptr;

	nvobj::persistent_ptr<int> p1;
	nvobj::persistent_ptr<int> p2;
	nvobj::persistent_ptr<int> p3;
};

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
}

void
test_dtor_after_tx_explicit_abort(nvobj::pool<struct root> &pop)
{
	try {
		nvobj::transaction::run(pop, [&] {
			pop.root()->c_explicit_abort_ptr =
				nvobj::make_persistent<C_explicit_abort>();
		});

		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &) {
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
				nvobj::transaction::abort(0);
			} catch (...) {
				/* ignore exception */
			}
		});

		UT_ASSERT(0);
	} catch (pmem::transaction_error &) {
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
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
				auto nested_opts =
					nvobj::transaction::options{};
				nested_opts.failure_behavior = nvobj::
					transaction::failure_behavior::abort;

				nvobj::transaction::run(
					pop, nested_opts,
					[&]() { UT_ASSERT(0); });
			} catch (pmem::transaction_error &) {
				UT_ASSERT(pmemobj_tx_stage() == TX_STAGE_WORK);
				UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
					  POBJ_TX_FAILURE_RETURN);

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

	nvobj::transaction::run(pop, [&] {
		nvobj::delete_persistent<int>(r->p1);
		r->p1 = nullptr;
	});
}

template <typename T>
void
test_tx_nested_behavior_scope(nvobj::pool<struct root> &pop)
{
	bool exception_thrown = false;

	auto r = pop.root();

	try {
		T to(pop);
		UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
			  POBJ_TX_FAILURE_RETURN);

		r->p1 = nvobj::make_persistent<int>();
		try {
			auto nested_opts = nvobj::transaction::options{};
			nested_opts.failure_behavior =
				nvobj::transaction::failure_behavior::abort;

			T to_nested(pop, nested_opts);
			UT_ASSERT(0);
		} catch (std::runtime_error &) {
			UT_ASSERT(pmemobj_tx_stage() == TX_STAGE_WORK);
			UT_ASSERT(pmemobj_tx_get_failure_behavior() ==
				  POBJ_TX_FAILURE_RETURN);

			exception_thrown = true;
		} catch (...) {
			UT_ASSERT(0);
		}

		if (std::is_same<T, nvobj::transaction::manual>::value)
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
		counter = 0;
		UT_ASSERT(exception_thrown);
		exception_thrown = false;

		if (std::is_same<T, nvobj::transaction::manual>::value)
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
			counter = 0;
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

	test_dtor_after_tx_abort(pop);
	test_nested_dtor_after_tx_abort(pop);

	test_tx_nested_behavior(pop);

	test_tx_nested_behavior_scope<nvobj::transaction::manual>(pop);
	test_tx_nested_behavior_scope<nvobj::transaction::automatic>(pop);

	test_tx_throw_no_abort_scope<nvobj::transaction::manual>(pop);
	test_tx_throw_no_abort_scope<nvobj::transaction::automatic>(pop);

	test_tx_automatic_destructor_throw(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
