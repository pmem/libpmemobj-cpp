/*
 * Copyright 2016-2020, Intel Corporation
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
 * Custom exceptions.
 */

#ifndef LIBPMEMOBJ_CPP_PEXCEPTIONS_HPP
#define LIBPMEMOBJ_CPP_PEXCEPTIONS_HPP

#include <stdexcept>
#include <string>
#include <system_error>

#include <libpmemobj/atomic_base.h>
#include <libpmemobj/base.h>

namespace pmem
{

namespace detail
{

/**
 * Return last libpmemobj error message as a std::string.
 */
inline std::string
errormsg(void)
{
#ifdef _WIN32
	return std::string(pmemobj_errormsgU());
#else
	return std::string(pmemobj_errormsg());
#endif
}
} /* namespace detail */

/**
 * Custom pool error class.
 *
 * Thrown when there is a runtime problem with some action on the
 * pool.
 */
class pool_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;

	pool_error &
	with_pmemobj_errormsg()
	{
		(*this) = pool_error(what() + std::string(": ") +
				     detail::errormsg());
		return *this;
	}
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a runtime problem with a transaction.
 */
class transaction_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;

	transaction_error &
	with_pmemobj_errormsg()
	{
		(*this) = transaction_error(what() + std::string(": ") +
					    detail::errormsg());
		return *this;
	}
};

/**
 * Custom lock error class.
 *
 * Thrown when there is a runtime system error with an operation
 * on a lock.
 */
class lock_error : public std::system_error {
public:
	using std::system_error::system_error;

	lock_error &
	with_pmemobj_errormsg()
	{
		(*this) = lock_error(code(),
				     what() + std::string(": ") +
					     detail::errormsg());
		return *this;
	}
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a transactional allocation error.
 */
class transaction_alloc_error : public transaction_error {
public:
	using transaction_error::transaction_error;

	transaction_alloc_error &
	with_pmemobj_errormsg()
	{
		(*this) = transaction_alloc_error(what() + std::string(": ") +
						  detail::errormsg());
		return *this;
	}
};

/**
 * Custom out of memory error class.
 *
 * Thrown when there is out of memory error inside of transaction.
 */
class transaction_out_of_memory : public transaction_alloc_error,
				  public std::bad_alloc {
public:
	using transaction_alloc_error::transaction_alloc_error;
	using transaction_alloc_error::what;

	transaction_out_of_memory &
	with_pmemobj_errormsg()
	{
		(*this) = transaction_out_of_memory(
			transaction_alloc_error::what() + std::string(": ") +
			detail::errormsg());
		return *this;
	}
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a transactional free error.
 */
class transaction_free_error : public transaction_alloc_error {
public:
	using transaction_alloc_error::transaction_alloc_error;

	transaction_free_error &
	with_pmemobj_errormsg()
	{
		(*this) = transaction_free_error(what() + std::string(": ") +
						 detail::errormsg());
		return *this;
	}
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is an error with the scope of the transaction.
 */
class transaction_scope_error : public std::logic_error {
public:
	using std::logic_error::logic_error;
};

/**
 * Custom transaction error class.
 *
 * Thrown on manual transaction abort.
 */
class manual_tx_abort : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * Custom layout error class.
 *
 * Thrown when data layout is different than expected by the library.
 */
class layout_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * Custom ctl error class.
 *
 * Thrown on ctl_[get|set|exec] failure.
 */
class ctl_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;

	ctl_error &
	with_pmemobj_errormsg()
	{
		(*this) = ctl_error(what() + std::string(": ") +
				    detail::errormsg());
		return *this;
	}
};

/**
 * Custom defrag error class.
 *
 * Thrown when the defragmentation process fails
 * (possibly in the middle of a run).
 */
class defrag_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;

	defrag_error(pobj_defrag_result result, const std::string &msg)
	    : std::runtime_error(msg), result(result)
	{
	}

	defrag_error &
	with_pmemobj_errormsg()
	{
		(*this) = defrag_error(result,
				       what() + std::string(": ") +
					       detail::errormsg());
		return *this;
	}

	/**
	 * Results of the defragmentation run.
	 *
	 * When failure occurs during the defragmentation,
	 * partial results will be stored in here.
	 */
	pobj_defrag_result result;
};

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_PEXCEPTIONS_HPP */
