/*
 * Copyright 2016-2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 * C++ pmemobj transactions.
 */

#ifndef LIBPMEMOBJ_CPP_TRANSACTION_HPP
#define LIBPMEMOBJ_CPP_TRANSACTION_HPP

#include <array>
#include <functional>
#include <string>
#include <vector>

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/tx_base.h>

namespace pmem
{

namespace obj
{

/**
 * C++ transaction handler class.
 *
 * This class is the pmemobj transaction handler. Scoped transactions
 * are handled through two internal classes: @ref manual and
 * @ref automatic.
 * - @ref manual transactions need to be committed manually, otherwise
 *	they will be aborted on object destruction.\n
 * - @ref automatic transactions are only available in C++17. They
 *	handle transaction commit/abort automatically.
 *
 * This class also exposes a closure-like transaction API, which is the
 * preferred way of handling transactions.
 *
 * This API should NOT be mixed with C transactions API. One issue is that
 * C++ callbacks registered using transaction::register_callback() would not
 * be called if C++ transaction is created inside C transaction.
 * The same is true if user calls pmemobj_tx_set_user_data() inside a C++
 * transaction.
 *
 * The typical usage example would be:
 * @snippet doc_snippets/transaction.cpp general_tx_example
 */
class transaction {
public:
	/**
	 * C++ manual scope transaction class.
	 *
	 * This class is one of pmemobj transaction handlers. All
	 * operations between creating and destroying the transaction
	 * object are treated as performed in a transaction block and
	 * can be rolled back. The manual transaction has to be
	 * committed explicitly otherwise it will abort.
	 *
	 * The locks are held for the entire duration of the transaction. They
	 * are released at the end of the scope, so within the `catch` block,
	 * they are already unlocked. If the cleanup action requires access to
	 * data within a critical section, the locks have to be manually
	 * acquired once again.
	 *
	 *The typical usage example would be:
	 * @snippet doc_snippets/transaction.cpp manual_tx_example
	 */
	class manual {
	public:
		/**
		 * RAII constructor with pmem resident locks.
		 *
		 * Start pmemobj transaction and add list of locks to
		 * new transaction. The list of locks may be empty.
		 *
		 * @param[in,out] pop pool object.
		 * @param[in,out] locks locks of obj::mutex or
		 *	obj::shared_mutex type.
		 *
		 * @throw pmem::transaction_error when pmemobj_tx_begin
		 * function or locks adding failed.
		 */
		template <typename... L>
		manual(obj::pool_base &pop, L &... locks)
		{
			int ret = 0;

			if (pmemobj_tx_stage() == TX_STAGE_NONE) {
				ret = pmemobj_tx_begin(pop.handle(), nullptr,
						       TX_PARAM_CB,
						       transaction::c_callback,
						       nullptr, TX_PARAM_NONE);
			} else {
				ret = pmemobj_tx_begin(pop.handle(), nullptr,
						       TX_PARAM_NONE);
			}

			if (ret != 0)
				throw pmem::transaction_error(
					"failed to start transaction")
					.with_pmemobj_errormsg();

			auto err = add_lock(locks...);

			if (err) {
				pmemobj_tx_abort(EINVAL);
				(void)pmemobj_tx_end();
				throw pmem::transaction_error(
					"failed to add lock")
					.with_pmemobj_errormsg();
			}
		}

		/**
		 * Destructor.
		 *
		 * End pmemobj transaction. If the transaction has not
		 * been committed before object destruction, an abort
		 * will be issued.
		 */
		~manual() noexcept
		{
			/* normal exit or with an active exception */
			if (pmemobj_tx_stage() == TX_STAGE_WORK)
				pmemobj_tx_abort(ECANCELED);

			(void)pmemobj_tx_end();
		}

		/**
		 * Deleted copy constructor.
		 */
		manual(const manual &p) = delete;

		/**
		 * Deleted move constructor.
		 */
		manual(const manual &&p) = delete;

		/**
		 * Deleted assignment operator.
		 */
		manual &operator=(const manual &p) = delete;

		/**
		 * Deleted move assignment operator.
		 */
		manual &operator=(manual &&p) = delete;
	};

/*
 * XXX The Microsoft compiler does not follow the ISO SD-6: SG10 Feature
 * Test Recommendations. "|| _MSC_VER >= 1900" is a workaround.
 */
#if __cpp_lib_uncaught_exceptions || _MSC_VER >= 1900
	/**
	 * C++ automatic scope transaction class.
	 *
	 * This class is one of pmemobj transaction handlers. All
	 * operations between creating and destroying the transaction
	 * object are treated as performed in a transaction block and
	 * can be rolled back. If you have a C++17 compliant compiler,
	 * the automatic transaction will commit and abort
	 * automatically depending on the context of object destruction.
	 *
	 * The locks are held for the entire duration of the transaction. They
	 * are released at the end of the scope, so within the `catch` block,
	 * they are already unlocked. If the cleanup action requires access to
	 * data within a critical section, the locks have to be manually
	 * acquired once again.
	 *
	 * The typical usage example would be:
	 * @snippet doc_snippets/transaction.cpp automatic_tx_example
	 */
	class automatic {
	public:
		/**
		 * RAII constructor with pmem resident locks.
		 *
		 * Start pmemobj transaction and add list of locks to
		 * new transaction. The list of locks may be empty.
		 *
		 * This class is only available if the
		 * `__cpp_lib_uncaught_exceptions` feature macro is
		 * defined. This is a C++17 feature.
		 *
		 * @param[in,out] pop pool object.
		 * @param[in,out] locks locks of obj::mutex or
		 *	obj::shared_mutex type.
		 *
		 * @throw pmem::transaction_error when pmemobj_tx_begin
		 * function or locks adding failed.
		 */
		template <typename... L>
		automatic(obj::pool_base &pop, L &... locks)
		    : tx_worker(pop, locks...)
		{
		}

		/**
		 * Destructor.
		 *
		 * End pmemobj transaction. Depending on the context
		 * of object destruction, the transaction will
		 * automatically be either committed or aborted.
		 *
		 * @throw pmem::transaction_error if the transaction got aborted
		 * without an active exception.
		 */
		~automatic() noexcept(false)
		{
			/* active exception, abort handled by tx_worker */
			if (exceptions.new_uncaught_exception())
				return;

			/* transaction ended normally */
			if (pmemobj_tx_stage() == TX_STAGE_WORK)
				pmemobj_tx_commit();
			/* transaction aborted, throw an exception */
			else if (pmemobj_tx_stage() == TX_STAGE_ONABORT ||
				 (pmemobj_tx_stage() == TX_STAGE_FINALLY &&
				  pmemobj_tx_errno() != 0))
				throw pmem::transaction_error(
					"Transaction aborted");
		}

		/**
		 * Deleted copy constructor.
		 */
		automatic(const automatic &p) = delete;

		/**
		 * Deleted move constructor.
		 */
		automatic(const automatic &&p) = delete;

		/**
		 * Deleted assignment operator.
		 */
		automatic &operator=(const automatic &p) = delete;

		/**
		 * Deleted move assignment operator.
		 */
		automatic &operator=(automatic &&p) = delete;

	private:
		/**
		 * Internal class for counting active exceptions.
		 */
		class uncaught_exception_counter {
		public:
			/**
			 * Default constructor.
			 *
			 * Sets the number of active exceptions on
			 * object creation.
			 */
			uncaught_exception_counter()
			    : count(std::uncaught_exceptions())
			{
			}

			/**
			 * Notifies is a new exception is being handled.
			 *
			 * @return true if a new exception was throw
			 *	in the scope of the object, false
			 *	otherwise.
			 */
			bool
			new_uncaught_exception()
			{
				return std::uncaught_exceptions() > this->count;
			}

		private:
			/**
			 * The number of active exceptions.
			 */
			int count;
		} exceptions;

		transaction::manual tx_worker;
	};
#endif /* __cpp_lib_uncaught_exceptions */

	/*
	 * Deleted default constructor.
	 */
	transaction() = delete;

	/**
	 * Default destructor.
	 *
	 * End pmemobj transaction. If the transaction has not been
	 * committed before object destruction, an abort will be issued.
	 */
	~transaction() noexcept = delete;

	/**
	 * Manually abort the current transaction.
	 *
	 * If called within an inner transaction, the outer transactions
	 * will also be aborted.
	 *
	 * @param[in] err the error to be reported as the reason of the
	 *	abort.
	 *
	 * @throw transaction_error if the transaction is in an invalid
	 *	state.
	 * @throw manual_tx_abort this exception is thrown to
	 *	signify a transaction abort.
	 */
	static void
	abort(int err)
	{
		if (pmemobj_tx_stage() != TX_STAGE_WORK)
			throw pmem::transaction_error("wrong stage for abort");

		pmemobj_tx_abort(err);
		throw pmem::manual_tx_abort("explicit abort " +
					    std::to_string(err));
	}

	/**
	 * Manually commit a transaction.
	 *
	 * It is the sole responsibility of the caller, that after the
	 * call to transaction::commit() no other operations are done
	 * within the transaction.
	 *
	 * @throw transaction_error on any errors with ending the
	 *	transaction.
	 */
	static void
	commit()
	{
		if (pmemobj_tx_stage() != TX_STAGE_WORK)
			throw pmem::transaction_error("wrong stage for commit");

		pmemobj_tx_commit();
	}

	static int
	error() noexcept
	{
		return pmemobj_tx_errno();
	}

	POBJ_CPP_DEPRECATED static int
	get_last_tx_error() noexcept
	{
		return transaction::error();
	}

	/**
	 * Execute a closure-like transaction and lock `locks`.
	 *
	 * The locks have to be persistent memory resident locks. An
	 * attempt to lock the locks will be made. If any of the
	 * specified locks is already locked, the method will block.
	 * The locks are held until the end of the transaction. The
	 * transaction does not have to be committed manually. Manual
	 * aborts will end the transaction with an active exception.
	 *
	 * If an exception is thrown within the transaction, it gets aborted
	 * and the exception is rethrown. Therefore extra care has to be taken
	 * with proper error handling.
	 *
	 * The locks are held for the entire duration of the transaction. They
	 * are released at the end of the scope, so within the `catch` block,
	 * they are already unlocked. If the cleanup action requires access to
	 * data within a critical section, the locks have to be manually
	 * acquired once again.
	 *
	 * @param[in,out] pool the pool in which the transaction will take
	 *	place.
	 * @param[in] tx an std::function<void ()> which will perform
	 *	operations within this transaction.
	 * @param[in,out] locks locks to be taken for the duration of
	 *	the transaction.
	 *
	 * @throw transaction_error on any error pertaining the execution
	 *	of the transaction.
	 * @throw manual_tx_abort on manual transaction abort.
	 */
	template <typename... Locks>
	static void
	run(pool_base &pool, std::function<void()> tx, Locks &... locks)
	{
		int ret = 0;

		if (pmemobj_tx_stage() == TX_STAGE_NONE) {
			ret = pmemobj_tx_begin(pool.handle(), nullptr,
					       TX_PARAM_CB,
					       transaction::c_callback, nullptr,
					       TX_PARAM_NONE);
		} else {
			ret = pmemobj_tx_begin(pool.handle(), nullptr,
					       TX_PARAM_NONE);
		}

		if (ret != 0)
			throw pmem::transaction_error(
				"failed to start transaction")
				.with_pmemobj_errormsg();

		auto err = add_lock(locks...);

		if (err) {
			pmemobj_tx_abort(err);
			(void)pmemobj_tx_end();
			throw pmem::transaction_error(
				"failed to add a lock to the transaction")
				.with_pmemobj_errormsg();
		}

		try {
			tx();
		} catch (manual_tx_abort &) {
			(void)pmemobj_tx_end();
			throw;
		} catch (...) {
			/* first exception caught */
			if (pmemobj_tx_stage() == TX_STAGE_WORK)
				pmemobj_tx_abort(ECANCELED);

			/* waterfall tx_end for outer tx */
			(void)pmemobj_tx_end();
			throw;
		}

		auto stage = pmemobj_tx_stage();

		if (stage == TX_STAGE_WORK) {
			pmemobj_tx_commit();
		} else if (stage == TX_STAGE_ONABORT) {
			(void)pmemobj_tx_end();
			throw pmem::transaction_error("transaction aborted");
		} else if (stage == TX_STAGE_NONE) {
			throw pmem::transaction_error(
				"transaction ended prematurely");
		}

		(void)pmemobj_tx_end();
	}

	template <typename... Locks>
	POBJ_CPP_DEPRECATED static void
	exec_tx(pool_base &pool, std::function<void()> tx, Locks &... locks)
	{
		transaction::run(pool, tx, locks...);
	}

	/**
	 * Takes a “snapshot” of given elements of type T number (1 by default),
	 * located at the given address ptr in the virtual memory space and
	 * saves it to the undo log. The application is then free to directly
	 * modify the object in that memory range. In case of a failure or
	 * abort, all the changes within this range will be rolled back. The
	 * supplied block of memory has to be within the pool registered in the
	 * transaction. This function must be called during transaction. This
	 * overload only participates in overload resolution of function
	 * template if T satisfies requirements of
	 * LIBPMEMOBJ_CPP_IS_TRIVIALLY_COPYABLE macro.
	 *
	 * @param[in] addr pointer to the first object to be snapshotted.
	 * @param[in] num number of elements to be snapshotted.
	 *
	 * @pre this function must be called during transaction.
	 *
	 * @throw transaction_error when snapshotting failed or if function
	 * wasn't called during transaction.
	 */
	template <
		typename T,
		typename std::enable_if<LIBPMEMOBJ_CPP_IS_TRIVIALLY_COPYABLE(T),
					T>::type * = nullptr>
	static void
	snapshot(const T *addr, size_t num = 1)
	{
		if (TX_STAGE_WORK != pmemobj_tx_stage())
			throw pmem::transaction_error(
				"wrong stage for taking a snapshot.");

		if (pmemobj_tx_add_range_direct(addr, sizeof(*addr) * num)) {
			if (errno == ENOMEM)
				throw pmem::transaction_out_of_memory(
					"Could not take a snapshot of given memory range.")
					.with_pmemobj_errormsg();
			else
				throw pmem::transaction_error(
					"Could not take a snapshot of given memory range.")
					.with_pmemobj_errormsg();
		}
	}

	/**
	 * Possible stages of a transaction, for every stage one or more
	 * callbacks can be registered.
	 */
	enum class stage {
		work = TX_STAGE_WORK,	      /* transaction in progress */
		oncommit = TX_STAGE_ONCOMMIT, /* successfully committed */
		onabort = TX_STAGE_ONABORT,   /* tx_begin failed or transaction
						 aborted */
		finally = TX_STAGE_FINALLY,   /* ready for cleanup */
	};

	/**
	 * Registers callback to be called on specified stage for the
	 * transaction. In case of nested transactions those callbacks
	 * are called when the outer most transaction enters a specified stage.
	 *
	 * @pre this function must be called during a transaction.
	 *
	 * @throw transaction_scope_error when called outside of a transaction
	 * scope
	 *
	 * The typical usage example would be:
	 * @snippet doc_snippets/transaction.cpp tx_callback_example
	 */
	static void
	register_callback(stage stg, std::function<void()> cb)
	{
		if (pmemobj_tx_stage() != TX_STAGE_WORK)
			throw pmem::transaction_scope_error(
				"register_callback must be called during a transaction");

		get_tx_data()->callbacks[static_cast<size_t>(stg)].push_back(
			cb);
	}

private:
	/**
	 * Recursively add locks to the active transaction.
	 *
	 * The locks are taken in the provided order.
	 *
	 * @param[in,out] lock the lock to add.
	 * @param[in,out] locks the rest of the locks to be added to the
	 *	active transaction.
	 *
	 * @return error number if adding any of the locks failed,
	 *	0 otherwise.
	 */
	template <typename L, typename... Locks>
	static int
	add_lock(L &lock, Locks &... locks) noexcept
	{
		auto err =
			pmemobj_tx_lock(lock.lock_type(), lock.native_handle());

		if (err)
			return err;

		return add_lock(locks...);
	}

	/**
	 * Method ending the recursive algorithm.
	 */
	static inline int
	add_lock() noexcept
	{
		return 0;
	}

	using callbacks_list_type = std::vector<std::function<void()>>;
	using callbacks_map_type =
		std::array<callbacks_list_type, MAX_TX_STAGE>;

	/**
	 * C-style function which is passed as callback to pmemobj_begin.
	 * It executes previously registered callbacks for all stages.
	 */
	static void
	c_callback(PMEMobjpool *pop, enum pobj_tx_stage obj_stage, void *arg)
	{
		/*
		 * We cannot do anything when in TX_STAGE_NONE because
		 * pmemobj_tx_get_user_data() can only be called when there is
		 * an active transaction.
		 */
		if (obj_stage == TX_STAGE_NONE)
			return;

		auto *data = static_cast<tx_data *>(pmemobj_tx_get_user_data());
		if (data == nullptr)
			return;

		for (auto &cb : data->callbacks[obj_stage])
			cb();

		/*
		 * Callback for TX_STAGE_FINALLY is called as the last one so we
		 * can free tx_data here
		 */
		if (obj_stage == TX_STAGE_FINALLY) {
			delete data;
			pmemobj_tx_set_user_data(NULL);
		}
	}

	/**
	 * This data is stored along with the pmemobj transaction data using
	 * pmemobj_tx_set_data().
	 */
	struct tx_data {
		callbacks_map_type callbacks;
	};

	/**
	 * Gets tx user data from pmemobj or creates it if this is a first
	 * call to this function inside a transaction.
	 */
	static tx_data *
	get_tx_data()
	{
		auto *data = static_cast<tx_data *>(pmemobj_tx_get_user_data());
		if (data == nullptr) {
			data = new tx_data;
			pmemobj_tx_set_user_data(data);
		}

		return data;
	}
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_TRANSACTION_HPP */
