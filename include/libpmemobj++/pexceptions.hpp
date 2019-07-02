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
 * Custom exceptions.
 */

#ifndef LIBPMEMOBJ_CPP_PEXCEPTIONS_HPP
#define LIBPMEMOBJ_CPP_PEXCEPTIONS_HPP

#include <stdexcept>
#include <system_error>

#include <libpmemobj.h>

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
}

/**
 * Custom pool error class.
 *
 * Thrown when there is a runtime problem with some action on the
 * pool.
 */
class pool_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a runtime problem with a transaction.
 */
class transaction_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
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
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a transactional allocation error.
 */
class transaction_alloc_error : public transaction_error {
public:
	using transaction_error::transaction_error;
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a transactional free error.
 */
class transaction_free_error : public transaction_alloc_error {
public:
	using transaction_alloc_error::transaction_alloc_error;
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
 * Custom ctl error class.
 *
 * Thrown on ctl_[get|set|exec] failure.
 */
class ctl_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

namespace obj
{

/**
 * Custom pool error class. Extends passed error mesage with
 * string obtained from pmemobj_errormsg().
 *
 * Thrown when there is a runtime problem with some action on the
 * pool.
 */
class pool_error : public pmem::pool_error {
public:
	pool_error(const std::string &msg = "")
	    : pmem::pool_error(msg + ": " + detail::errormsg())
	{
	}
};

/**
 * Custom transaction error class. Extends passed error mesage with
 * string obtained from pmemobj_errormsg().
 *
 * Thrown when there is a runtime problem with a transaction.
 */
class transaction_error : public pmem::transaction_error {
public:
	transaction_error(const std::string &msg = "")
	    : pmem::transaction_error(msg + ": " + detail::errormsg())
	{
	}
};

/**
 * Custom lock error class. Extends passed error mesage with
 * string obtained from pmemobj_errormsg().
 *
 * Thrown when there is a runtime system error with an operation
 * on a lock.
 */
class lock_error : public pmem::lock_error {
public:
	lock_error(int ev, const std::error_category &ecat,
		   const std::string &msg = "")
	    : pmem::lock_error(ev, ecat, msg + ": " + detail::errormsg())
	{
	}
};

/**
 * Custom transaction error class. Extends passed error mesage with
 * string obtained from pmemobj_errormsg().
 *
 * Thrown when there is a transactional allocation error.
 */
class transaction_alloc_error : public pmem::transaction_alloc_error {
public:
	transaction_alloc_error(const std::string &msg = "")
	    : pmem::transaction_alloc_error(msg + ": " + detail::errormsg())
	{
	}
};

/**
 * Custom transaction error class. Extends passed error mesage with
 * string obtained from pmemobj_errormsg().
 *
 * Thrown when there is a transactional free error.
 */
class transaction_free_error : public pmem::transaction_free_error {
public:
	transaction_free_error(const std::string &msg = "")
	    : pmem::transaction_free_error(msg + ": " + detail::errormsg())
	{
	}
};

/**
 * Custom ctl error class. Extends passed error mesage with
 * string obtained from pmemobj_errormsg().
 *
 * Thrown on ctl_[get|set|exec] failure.
 */
class ctl_error : public pmem::ctl_error {
public:
	ctl_error(const std::string &msg = "")
	    : pmem::ctl_error(msg + ": " + detail::errormsg())
	{
	}
};
}

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_PEXCEPTIONS_HPP */
