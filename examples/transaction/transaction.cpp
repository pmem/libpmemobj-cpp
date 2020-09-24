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
				assert(proot->count == 2);
				throw;
			}
		});
	} catch (pmem::transaction_error &) {
		/* an internal transaction error occurred, tx aborted
		 * reacquire locks if necessary */
		assert(proot->count == 0);
	} catch (...) {
		/* some other exception thrown, tx aborted
		 * reacquire locks if necessary */
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
		auto pop = pool_base(pmemobj_pool_by_ptr(this));

		/* Using a basic instead of flat one, would lead to an
		 * application terminate. The reason is that allocating the char
		 * array leads to an out-of-memory error. For basic_transaction,
		 * this error will cause the transaction to abort. For
		 * flat_transaction, there will be no immediate abort, but only
		 * an exception. This exception will be propagated to the
		 * outermost transaction and only then the abort will happen.
		 * This allows C++ to call ptr1 in A class destructor and unwind
		 * the stack correctly.
		 */
		flat_transaction::run(pop, [&] { ptr = make_persistent<T>(); });
	}

	~simple_ptr()
	{
		delete_persistent<T>(ptr);
	}

	persistent_ptr<T> ptr;
};

struct A {
	A() : ptr1(), ptr2()
	{
	}

	simple_ptr<int> ptr1;
	simple_ptr<char[(1ULL << 30)]> ptr2;
};

void
tx_nested_struct_example()
{
	/* pool root structure */
	struct root {
		persistent_ptr<A> ptr;
	};

	/* create a pmemobj pool */
	auto pop = pool<root>::create("poolfile", "layout", PMEMOBJ_MIN_POOL);
	auto proot = pop.root();

	try {
		/* Here, we could have either basic_transaction or
		 * flat_transaction. */
		flat_transaction::run(
			pop, [&] { proot->ptr = make_persistent<A>(); });
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

			/* Even if there is no explcit commit inner_flat tx will
			 * not abort. This is true even if
			 * flat_transaction::manual is destroyed because of an
			 * active exception.
			 */
		}

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
