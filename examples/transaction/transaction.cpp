// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2020, Intel Corporation */

/*
 * transaction.cpp -- C++ documentation snippets.
 */

/*
 * The following might be necessary to compile the examples on older compilers.
 */
#if !defined(__cpp_lib_uncaught_exceptions) && !defined(_MSC_VER) ||           \
	(_MSC_VER < 1900)

#define __cpp_lib_uncaught_exceptions 201411
namespace std
{

int
uncaught_exceptions() noexcept
{
	return 0;
}

} /* namespace std */
#endif /* __cpp_lib_uncaught_exceptions */

//! [general_tx_example]
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
general_tx_example()
{
	/* pool root structure */
	struct root {
		mutex pmutex;
		shared_mutex shared_pmutex;
		p<int> count;
		persistent_ptr<root> another_root;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	/* typical usage schemes */
	try {
		/* take locks and start a transaction */
		transaction::run(
			pop,
			[&]() {
				/* atomically allocate objects */
				proot->another_root = make_persistent<root>();

				/* atomically modify objects */
				proot->count++;
			},
			proot->pmutex, proot->shared_pmutex);
	} catch (pmem::transaction_error &) {
		/* a transaction error occurred, transaction got aborted
		 * reacquire locks if necessary */
	} catch (...) {
		/* some other exception got propagated from within the tx
		 * reacquire locks if necessary */
	}
}
//! [general_tx_example]

//! [manual_tx_example]
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

int
manual_tx_example()
{
	/* pool root structure */
	struct root {
		mutex pmutex;
		shared_mutex shared_pmutex;
		p<int> count;
		persistent_ptr<root> another_root;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);

	auto proot = pop.root();

	try {
		transaction::manual tx(pop, proot->pmutex,
				       proot->shared_pmutex);

		/* atomically allocate objects */
		proot->another_root = make_persistent<root>();

		/* atomically modify objects */
		proot->count++;

		/* It's necessary to commit the transaction manually and
		 * it has to be the last operation in the transaction. */
		transaction::commit();
	} catch (pmem::transaction_error &) {
		/* an internal transaction error occurred, tx aborted
		 * reacquire locks if necessary */
	} catch (...) {
		/* some other exception thrown, tx aborted
		 * reacquire locks if necessary */
	}

	/* In complex cases with library calls, remember to check the status of
	 * the previous transaction. */
	return transaction::error();
}
//! [manual_tx_example]

//! [automatic_tx_example]
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

int
automatic_tx_example()
{
	/* pool root structure */
	struct root {
		mutex pmutex;
		shared_mutex shared_pmutex;
		p<int> count;
		persistent_ptr<root> another_root;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	try {
		transaction::automatic tx(pop, proot->pmutex,
					  proot->shared_pmutex);

		/* atomically allocate objects */
		proot->another_root = make_persistent<root>();

		/* atomically modify objects */
		proot->count++;

		/* manual transaction commit is no longer necessary */
	} catch (pmem::transaction_error &) {
		/* an internal transaction error occurred, tx aborted
		 * reacquire locks if necessary */
	} catch (...) {
		/* some other exception thrown, tx aborted
		 * reacquire locks if necessary */
	}

	/* In complex cases with library calls, remember to check the status of
	 * the previous transaction. */
	return transaction::error();
}
//! [automatic_tx_example]

//! [tx_callback_example]
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
tx_callback_example()
{
	/* pool root structure */
	struct root {
		p<int> count;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);

	bool cb_called = false;
	auto internal_tx_function = [&] {
		/* callbacks can be registered even in inner transaction but
		 * will be called when outer transaction ends */
		transaction::run(pop, [&] {
			transaction::register_callback(
				transaction::stage::oncommit,
				[&] { cb_called = true; });
		});

		/* cb_called is false here if internal_tx_function is called
		 * inside another transaction */
	};

	try {
		transaction::run(pop, [&] { internal_tx_function(); });

		/* cb_called == true if transaction ended successfully */
	} catch (pmem::transaction_error &) {
		/* an internal transaction error occurred, tx aborted
		 * reacquire locks if necessary */
	} catch (...) {
		/* some other exception thrown, tx aborted
		 * reacquire locks if necessary */
	}
}
//! [tx_callback_example]

//! [tx_flat_example]
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

void
tx_flat_example()
{
	/* pool root structure */
	struct root {
		p<int> count;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	try {
		flat_transaction::run(pop, [&] {
			proot->count++;

			try {
				flat_transaction::run(pop, [&] {
					proot->count++;
					throw std::runtime_error("some error");
				});
			} catch (...) {
				/* Transaction is not aborted yet (unlike for
				 * basic_transaction). */
				assert(pmemobj_tx_stage() == TX_STAGE_WORK);
				assert(proot->count == 2);
				throw;
			}
		});
	} catch (pmem::transaction_error &) {
		/* An internal transaction error occurred, outer tx aborted just
		 * now. Reacquire locks if necessary. */
		assert(proot->count == 0);
	} catch (...) {
		/* Some other exception thrown, outer tx aborted just now.
		 * Reacquire locks if necessary. */
		assert(proot->count == 0);
	}
}
//! [tx_flat_example]

//! [tx_nested_struct_example]
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

template <typename T>
struct simple_ptr {
	simple_ptr()
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);
		ptr = make_persistent<T>();
	}

	~simple_ptr()
	{
		assert(pmemobj_tx_stage() == TX_STAGE_WORK);

		try {
			delete_persistent<T>(ptr);
		} catch (pmem::transaction_free_error &e) {
			std::cerr << e.what() << std::endl;
			std::terminate();
		}
	}

	persistent_ptr<T> ptr;
};

/**
 * This struct holds two simple_ptr. It presents problems when using
 * basic_transaction in case of transactional function abort. */
struct A {
	A() : ptr1(), ptr2()
	{
	}

	simple_ptr<int> ptr1;
	simple_ptr<char[(1ULL << 30)]> ptr2;
};

/**
 * This struct holds two simple_ptr. It presents problems when throwing
 * exception from within basic_transaction. */
struct B {
	B() : ptr1(), ptr2()
	{
		auto pop = pool_base(pmemobj_pool_by_ptr(this));

		// It would result in a crash!
		// basic_transaction::run(pop, [&]{ throw
		// std::runtime_error("Error"); });

		flat_transaction::run(
			pop, [&] { throw std::runtime_error("Error"); });
	}

	simple_ptr<int> ptr1;
	simple_ptr<int> ptr2;
};

void
tx_nested_struct_example()
{
	/* pool root structure */
	struct root {
		persistent_ptr<A> ptrA;
		persistent_ptr<B> ptrB;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	auto create_a = [&] { proot->ptrA = make_persistent<A>(); };
	auto create_b = [&] { proot->ptrB = make_persistent<B>(); };

	try {
		// It would result in a crash!
		// basic_transaction::run(pop, create_a);

		flat_transaction::run(pop, create_a);

		/* To see why flat_transaction is necessary let's
		 * consider what happens when calling A ctor. The call stack
		 * will look like this:
		 *
		 *  | ptr2 ctor |
		 *  |-----------|
		 *  | ptr1 ctor |
		 *  |-----------|
		 *  |  A ctor   |
		 *
		 * Since ptr2 is a pointer to some huge array of elements,
		 * calling ptr2 ctor will most likely result in make_persistent
		 * throwing an exception (due to out of memory). This exception
		 * will, in turn, cause stack unwinding - already constructed
		 * elements must be destroyed (in this example ptr1 destructor
		 * will be called).
		 *
		 * If we'd use basic_transaction the allocation failure, apart
		 * from throwing an exception, would also cause the transaction
		 * to abort (by default, in basic_transaction, all transactional
		 * functions failures cause tx abort). This is problematic since
		 * the ptr1 destructor, which is called during stack unwinding,
		 * expects the transaction to be in WORK stage (and the actual
		 * stage is ABORTED). As a result the application will fail on
		 * assert (and probably crash in NDEBUG mode).
		 *
		 * Now, consider what will happen if we'd use flat_transaction
		 * instead. In this case, make_persistent failure will not abort
		 * the transaction, it will only result in an exception. This
		 * means that the transaction is still in WORK stage during
		 * stack unwinding. Only after it completes, the transaction is
		 * aborted (it's happening at the outermost level, when exiting
		 * create_a lambda).
		 */
	} catch (std::runtime_error &) {
	}

	try {
		basic_transaction::run(pop, create_b);
		flat_transaction::run(pop, create_b);

		/* Running create_b can be done both within basic and flat
		 * transaction. However, note that the transaction used in the B
		 * constructor MUST be a flat_transaction. This is because
		 * flat_transaction does not abort immediately when catching an
		 * exception. Instead it passes it to the outermost transaction
		 * - the abort is performed at that outermost level. In case of
		 * a basic_transaction the abort would be done within the B ctor
		 * and it would result in the same problems as with the previous
		 * example.
		 */
	} catch (std::runtime_error &) {
	}
}
//! [tx_nested_struct_example]

//! [manual_flat_tx_example]
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem::obj;

int
manual_flat_tx_example()
{
	/* pool root structure */
	struct root {
		mutex pmutex;
		shared_mutex shared_pmutex;
		p<int> count;
		persistent_ptr<root> another_root;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);

	auto proot = pop.root();

	try {
		flat_transaction::manual tx(pop, proot->pmutex);

		/* atomically allocate objects */
		proot->another_root = make_persistent<root>();

		{
			flat_transaction::manual inner_tx(pop,
							  proot->shared_pmutex);

			/* atomically modify objects */
			proot->count++;

			/* OPTIONAL */
			// transaction::commit();

			/* Even if there is no explicit commit inner_tx will
			 * not abort. This is true even if
			 * flat_transaction::manual is destroyed because of an
			 * active exception. For basic_transaction::manual you
			 * have to call commit() at each level (as many times as
			 * there are manual transaction objects). In case of
			 * a flat_transaction, the commit has to be called only
			 * once, at the outermost level.
			 */
		}

		/* It's necessary to commit the transaction manually and
		 * it has to be the last operation in the transaction. */
		transaction::commit();
	} catch (pmem::transaction_error &) {
		/* An internal transaction error occurred, outer tx aborted just
		 * now. Reacquire locks if necessary, */
	} catch (...) {
		/* Some other exception thrown, outer tx aborted just now.
		 * Reacquire locks if necessary. */
	}

	/* In complex cases with library calls, remember to check the status of
	 * the last transaction. */
	return transaction::error();
}
//! [manual_flat_tx_example]

int
main()
{
	try {
		general_tx_example();
		manual_tx_example();
		automatic_tx_example();
		tx_callback_example();
		tx_flat_example();
		tx_nested_struct_example();
		manual_flat_tx_example();
	} catch (const std::exception &e) {
		std::cerr << "Exception " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
