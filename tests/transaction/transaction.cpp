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

/*
 * obj_cpp_transaction.cpp -- cpp transaction test
 */

#include "unittest.hpp"

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array_atomic.hpp>
#include <libpmemobj++/mutex.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/shared_mutex.hpp>

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

#define LAYOUT "cpp"
#define POOL_SIZE 2 * PMEMOBJ_MIN_POOL

namespace nvobj = pmem::obj;

namespace
{

struct foo {
	nvobj::p<int> bar;
	nvobj::shared_mutex smtx;
};

struct root {
	nvobj::persistent_ptr<foo> pfoo;
	nvobj::persistent_ptr<nvobj::p<int>> parr;
	nvobj::mutex mtx;
	nvobj::shared_mutex shared_mutex;
};

void
fake_commit()
{
}

void
real_commit()
{
	nvobj::transaction::commit();
}

/*
 * Callable object class.
 */
class transaction_test {
public:
	/*
	 * Constructor.
	 */
	transaction_test(nvobj::pool<root> &pop_) : pop(pop_)
	{
	}

	/*
	 * The transaction worker.
	 */
	void
	operator()()
	{
		auto rootp = this->pop.root();

		if (rootp->pfoo == nullptr)
			rootp->pfoo = nvobj::make_persistent<foo>();

		rootp->pfoo->bar = 42;
	}

private:
	nvobj::pool<root> &pop;
};

/*
 * do_transaction -- internal C-style function transaction.
 */
void
do_transaction(nvobj::pool<root> &pop)
{
	auto rootp = pop.root();

	rootp->parr = nvobj::make_persistent<nvobj::p<int>>();

	*rootp->parr.get() = 5;
}

/*
 * Closure tests.
 */

/*
 * test_tx_no_throw_no_abort -- test transaction without exceptions and aborts
 */
void
test_tx_no_throw_no_abort(nvobj::pool<root> &pop)
{
	auto rootp = pop.root();

	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		nvobj::transaction::run(pop, [&]() {
			rootp->pfoo = nvobj::make_persistent<foo>();
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(rootp->pfoo != nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		nvobj::transaction::run(
			pop, std::bind(do_transaction, std::ref(pop)),
			rootp->mtx);
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(rootp->pfoo != nullptr);
	UT_ASSERT(rootp->parr != nullptr);
	UT_ASSERTeq(*rootp->parr.get(), 5);

	try {
		nvobj::transaction::run(pop, transaction_test(pop), rootp->mtx,
					rootp->pfoo->smtx);
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(rootp->pfoo != nullptr);
	UT_ASSERT(rootp->parr != nullptr);
	UT_ASSERTeq(*rootp->parr.get(), 5);
	UT_ASSERTeq(rootp->pfoo->bar, 42);

	try {
		nvobj::transaction::run(pop, [&]() {
			nvobj::delete_persistent<foo>(rootp->pfoo);
			nvobj::delete_persistent<nvobj::p<int>>(rootp->parr);
			rootp->pfoo = nullptr;
			rootp->parr = nullptr;
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);
}

static bool
test_shared_mutex_self_deadlock()
{
	/*
	 * Starting transaction with already taken shared_lock should fail.
	 *
	 * However:
	 *  - pmemobj prior to 1.5.1 has a bug (see pmem/pmdk#3536) which
	 *    corrupts mutex state by unlocking it when it shouldn't
	 *  - shared_mutexes (rwlocks), as implemented by pmemobj, do not detect
	 *    self-deadlocks on Windows
	 */
#if TESTS_LIBPMEMOBJ_VERSION < 0x010501 || defined(_WIN32)
	return false;
#else
	return true;
#endif
}

/*
 * test_tx_throw_no_abort -- test transaction with exceptions and no aborts
 */
void
test_tx_throw_no_abort(nvobj::pool<root> &pop)
{
	auto rootp = pop.root();

	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&]() {
			rootp->pfoo = nvobj::make_persistent<foo>();
			throw std::runtime_error("error");
		});
	} catch (std::runtime_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	exception_thrown = false;
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		nvobj::transaction::run(pop, [&]() {
			rootp->pfoo = nvobj::make_persistent<foo>();
			nvobj::transaction::run(pop, [&]() {
				throw std::runtime_error("error");
			});
		});
	} catch (std::runtime_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	exception_thrown = false;
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		nvobj::transaction::run(pop, [&]() {
			rootp->pfoo = nvobj::make_persistent<foo>();
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
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	if (test_shared_mutex_self_deadlock()) {
		exception_thrown = false;
		rootp->shared_mutex.lock();
		try {
			nvobj::transaction::run(
				pop, [&]() {}, rootp->shared_mutex);
		} catch (pmem::transaction_error &) {
			exception_thrown = true;
		} catch (...) {
			UT_ASSERT(0);
		}

		UT_ASSERT(exception_thrown);
		rootp->shared_mutex.unlock();
	}
}

/*
 * test_tx_no_throw_abort -- test transaction with an abort and no exceptions
 */
void
test_tx_no_throw_abort(nvobj::pool<root> &pop)
{
	auto rootp = pop.root();

	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	bool exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&]() {
			rootp->pfoo = nvobj::make_persistent<foo>();
			nvobj::transaction::abort(-1);
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	exception_thrown = false;
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		nvobj::transaction::run(pop, [&]() {
			rootp->pfoo = nvobj::make_persistent<foo>();
			nvobj::transaction::run(
				pop, [&]() { nvobj::transaction::abort(-1); });
		});
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	exception_thrown = false;
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		nvobj::transaction::run(pop, [&]() {
			rootp->pfoo = nvobj::make_persistent<foo>();
			try {
				nvobj::transaction::run(pop, [&]() {
					nvobj::transaction::abort(-1);
				});
			} catch (pmem::manual_tx_abort &) {
				exception_thrown = true;
			} catch (...) {
				UT_ASSERT(0);
			}
		});
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERT(exception_thrown);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);
}

/*
 * Scoped tests.
 */

/*
 * test_tx_no_throw_no_abort_scope -- test transaction without exceptions
 *	and aborts
 */
template <typename T>
void
test_tx_no_throw_no_abort_scope(nvobj::pool<root> &pop,
				std::function<void()> commit)
{
	auto rootp = pop.root();

	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		T to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		commit();
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(rootp->pfoo != nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		T to(pop, rootp->mtx);
		do_transaction(pop);
		commit();
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(rootp->pfoo != nullptr);
	UT_ASSERT(rootp->parr != nullptr);
	UT_ASSERTeq(*rootp->parr.get(), 5);

	try {
		T to(pop, rootp->mtx, rootp->pfoo->smtx);
		transaction_test tt(pop);
		tt.operator()();
		commit();
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(rootp->pfoo != nullptr);
	UT_ASSERT(rootp->parr != nullptr);
	UT_ASSERTeq(*rootp->parr.get(), 5);
	UT_ASSERTeq(rootp->pfoo->bar, 42);

	try {
		T to(pop);
		nvobj::delete_persistent<foo>(rootp->pfoo);
		nvobj::delete_persistent<nvobj::p<int>>(rootp->parr);
		rootp->pfoo = nullptr;
		rootp->parr = nullptr;
		commit();
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);
}

/*
 * test_tx_throw_no_abort_scope -- test transaction with exceptions
 *	and no aborts
 */
template <typename T>
void
test_tx_throw_no_abort_scope(nvobj::pool<root> &pop)
{
	auto rootp = pop.root();

	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	bool exception_thrown = false;
	try {
		counter = 0;
		T to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		counter = 1;
		throw std::runtime_error("error");
	} catch (std::runtime_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	UT_ASSERT(exception_thrown);
	exception_thrown = false;
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		counter = 0;
		T to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		{
			T to_nested(pop);
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
	exception_thrown = false;
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		counter = 0;
		T to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
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
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	/* commiting non-existent transaction should fail with an exception */
	exception_thrown = false;
	try {
		nvobj::transaction::commit();
	} catch (pmem::transaction_error &te) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);

	if (test_shared_mutex_self_deadlock()) {
		exception_thrown = false;
		rootp->shared_mutex.lock();
		try {
			T t(pop, rootp->shared_mutex);
		} catch (pmem::transaction_error &) {
			exception_thrown = true;
		} catch (...) {
			UT_ASSERT(0);
		}

		UT_ASSERTeq(nvobj::transaction::error(), EINVAL);
		UT_ASSERT(exception_thrown);
		rootp->shared_mutex.unlock();
	}
}

/*
 * test_tx_no_throw_abort_scope -- test transaction with an abort
 *	and no exceptions
 */
template <typename T>
void
test_tx_no_throw_abort_scope(nvobj::pool<root> &pop)
{
	auto rootp = pop.root();

	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	bool exception_thrown = false;
	try {
		counter = 0;
		T to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		counter = 1;
		nvobj::transaction::abort(ECANCELED);
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	UT_ASSERT(exception_thrown);
	exception_thrown = false;
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		counter = 0;
		T to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		{
			T to_nested(pop);
			counter = 1;
			nvobj::transaction::abort(EINVAL);
		}
	} catch (pmem::manual_tx_abort &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), EINVAL);
	UT_ASSERT(exception_thrown);
	exception_thrown = false;
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		counter = 0;
		T to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		try {
			T to_nested(pop);
			counter = 1;
			nvobj::transaction::abort(-1);
		} catch (pmem::manual_tx_abort &) {
			exception_thrown = true;
		} catch (...) {
			UT_ASSERT(0);
		}
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), -1);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);
}

/*
 * test_tx_automatic_destructor_throw -- test transaction with a C tx_abort
 *	and no exceptions
 */
void
test_tx_automatic_destructor_throw(nvobj::pool<root> &pop)
{
	auto rootp = pop.root();

	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	bool exception_thrown = false;
	try {
		nvobj::transaction::automatic to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		pmemobj_tx_abort(ECANCELED);
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	exception_thrown = false;
	try {
		nvobj::transaction::automatic to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		pmemobj_tx_abort(ECANCELED);
		pmemobj_tx_process(); /* move to finally */
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	exception_thrown = false;
	try {
		nvobj::transaction::automatic to(pop);
		pmemobj_tx_commit();
		pmemobj_tx_process(); /* move to finally */
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), 0);
	UT_ASSERT(!exception_thrown);

	counter = 0;
	try {
		nvobj::transaction::automatic to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		try {
			nvobj::transaction::automatic to_nested(pop);
			pmemobj_tx_abort(-1);
		} catch (pmem::transaction_error &) {
			/*verify the exception only */
			counter = 1;
			throw;
		} catch (...) {
			UT_ASSERT(0);
		}
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), -1);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		nvobj::transaction::automatic to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
		try {
			nvobj::transaction::automatic to_nested(pop);
			pmemobj_tx_abort(-1);
		} catch (pmem::transaction_error &) {
			/*verify the exception only */
		} catch (...) {
			UT_ASSERT(0);
		}
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}

	UT_ASSERTeq(nvobj::transaction::error(), -1);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);

	try {
		counter = 0;
		nvobj::transaction::automatic to(pop);
		rootp->pfoo = nvobj::make_persistent<foo>();
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

	/* the transaction will be aborted silently */
	UT_ASSERTeq(nvobj::transaction::error(), ECANCELED);
	UT_ASSERT(exception_thrown);
	UT_ASSERT(rootp->pfoo == nullptr);
	UT_ASSERT(rootp->parr == nullptr);
}

/*
 * test_tx_snapshot -- 1) Check if transaction_error is thrown, when snapshot()
 * is not called from transaction.
 * 2) Check if transaction_error is thrown, when internal call to
 * pmemobj_tx_add_range_direct() failed.
 * 3) Check if assigning value to pmem object is valid under pmemcheck when
 * object was snapshotted beforehand.
 * 4) Check if snapshotted value was rolled back in case of transacion abort.
 */
void
test_tx_snapshot(nvobj::pool<root> &pop)
{
	nvobj::persistent_ptr<char[]> parr;
	try {
		nvobj::make_persistent_atomic<char[]>(pop, parr, 5);
	} catch (...) {
		UT_ASSERT(0);
	}

	bool exception_thrown = false;
	try {
		nvobj::transaction::snapshot<char>(parr.get(), 5);
		UT_ASSERT(0);
	} catch (std::bad_alloc &) {
		UT_ASSERT(0);
	} catch (pmem::transaction_error &) {
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);

	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::transaction::snapshot<char>(parr.get(),
							   POOL_SIZE);
		});
		UT_ASSERT(0);
	} catch (pmem::transaction_error &e) {
		(void)e.what();
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);

	nvobj::persistent_ptr<char[]> p1;

	try {
		nvobj::transaction::run(pop, [&] {
			p1 = nvobj::make_persistent<char[]>(POOL_SIZE / 2);
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	/* OOM handling */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::transaction::snapshot<char>(p1.get(),
							   POOL_SIZE / 2);
		});
		UT_ASSERT(0);
	} catch (std::bad_alloc &e) {
		(void)e.what();
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);

	/* OOM handling */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::transaction::snapshot<char>(p1.get(),
							   POOL_SIZE / 2);
		});
		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &e) {
		(void)e.what();
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);

	/* OOM handling */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			pmem::detail::conditional_add_to_tx<char>(
				p1.get(), POOL_SIZE / 2);
		});
		UT_ASSERT(0);
	} catch (pmem::transaction_alloc_error &e) {
		(void)e.what();
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);

	/* OOM handling */
	exception_thrown = false;
	try {
		nvobj::transaction::run(pop, [&] {
			pmem::detail::conditional_add_to_tx<char>(
				p1.get(), POOL_SIZE / 2);
		});
		UT_ASSERT(0);
	} catch (std::bad_alloc &e) {
		(void)e.what();
		exception_thrown = true;
	} catch (...) {
		UT_ASSERT(0);
	}
	UT_ASSERT(exception_thrown);

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::transaction::snapshot<char>(parr.get(), 5);
			for (int i = 0; i < 5; ++i)
				parr[i] = 1; /* no pmemcheck errors */
		});
	} catch (...) {
		UT_ASSERT(0);
	}

	try {
		nvobj::transaction::run(pop, [&] {
			nvobj::transaction::snapshot(parr.get(), 5);
			for (int i = 0; i < 5; ++i)
				parr[i] = 2;
			nvobj::transaction::abort(-1);
		});
		UT_ASSERT(0);
	} catch (pmem::manual_tx_abort &) {
		for (int i = 0; i < 5; ++i)
			UT_ASSERT(parr[i] == 1); /* check rolled back values */
	} catch (...) {
		UT_ASSERT(0);
	}
}
}

int
main(int argc, char *argv[])
{
	START();

	if (argc != 2)
		UT_FATAL("usage: %s file-name", argv[0]);

	const char *path = argv[1];

	nvobj::pool<root> pop;
	try {
		pop = nvobj::pool<root>::create(path, LAYOUT, POOL_SIZE,
						S_IWUSR | S_IRUSR);
	} catch (...) {
		UT_FATAL("!pmemobj_create: %s", path);
	}

	test_tx_no_throw_no_abort(pop);
	test_tx_throw_no_abort(pop);
	test_tx_no_throw_abort(pop);

	test_tx_no_throw_no_abort_scope<nvobj::transaction::manual>(
		pop, real_commit);
	test_tx_throw_no_abort_scope<nvobj::transaction::manual>(pop);
	test_tx_no_throw_abort_scope<nvobj::transaction::manual>(pop);

	test_tx_no_throw_no_abort_scope<nvobj::transaction::automatic>(
		pop, fake_commit);
	test_tx_throw_no_abort_scope<nvobj::transaction::automatic>(pop);
	test_tx_no_throw_abort_scope<nvobj::transaction::automatic>(pop);
	test_tx_automatic_destructor_throw(pop);

	test_tx_snapshot(pop);

	pop.close();

	return 0;
}
