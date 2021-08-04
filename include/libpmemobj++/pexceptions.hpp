// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2021, Intel Corporation */

/**
 * @file
 * Custom pmem exceptions.
 */

#ifndef LIBPMEMOBJ_CPP_PEXCEPTIONS_HPP
#define LIBPMEMOBJ_CPP_PEXCEPTIONS_HPP

#include <stdexcept>
#include <string>
#include <system_error>
#include <tuple>

#include <libpmemobj/atomic_base.h>
#include <libpmemobj/base.h>

#include <libpmemobj++/detail/integer_sequence.hpp>

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

/**
 * Helper for exception_with_errormsg.
 *
 * Accepts tuple with arguments and index_sequence which correspond
 * to all but last argument. The last element in tuple is assumed to
 * be an error message. Before passing it to ExcT constructor it is merged
 * with detail::errormsg()
 */
template <typename ExcT, size_t... I, typename Tuple>
ExcT
exception_with_errormsg_helper(index_sequence<I...>, Tuple &&args)
{
	static_assert(std::is_rvalue_reference<decltype(args)>::value,
		      "args should be rvalue reference!");

	auto msg = std::get<sizeof...(I)>(std::move(args)) + std::string(": ") +
		detail::errormsg();
	return ExcT(std::get<I>(std::move(args))..., std::move(msg));
}

/**
 * Generic error message decorator for pmemobj-based exceptions.
 *
 * It accepts arbitrary number of parameters. The last parameter must be an
 * error message.
 */
template <class ExcT, typename... Args>
ExcT
exception_with_errormsg(Args &&... args)
{
	return exception_with_errormsg_helper<ExcT>(
		make_index_sequence<sizeof...(Args) - 1>{},
		std::forward_as_tuple(std::forward<Args>(args)...));
}
} /* namespace detail */

/**
 * Custom pool error class.
 *
 * Thrown when there is a runtime problem with some action on the
 * pool.
 * @ingroup exceptions
 */
class pool_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * Custom pool error class.
 *
 * Thrown when there is an invalid argument passed to create/open pool.
 * @ingroup exceptions
 */
class pool_invalid_argument : public pool_error {
public:
	using pool_error::pool_error;
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a runtime problem with a transaction.
 * @ingroup exceptions
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
 * @ingroup exceptions
 */
class lock_error : public std::system_error {
public:
	using std::system_error::system_error;
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a transactional allocation error.
 * @ingroup exceptions
 */
class transaction_alloc_error : public transaction_error {
public:
	using transaction_error::transaction_error;
};

/**
 * Custom out of memory error class.
 *
 * Thrown when there is out of memory error inside of transaction.
 * @ingroup exceptions
 */
class transaction_out_of_memory : public transaction_alloc_error,
				  public std::bad_alloc {
public:
	using transaction_alloc_error::transaction_alloc_error;
	using transaction_alloc_error::what;
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is a transactional free error.
 * @ingroup exceptions
 */
class transaction_free_error : public transaction_alloc_error {
public:
	using transaction_alloc_error::transaction_alloc_error;
};

/**
 * Custom transaction error class.
 *
 * Thrown when there is an error with the scope of the transaction.
 * @ingroup exceptions
 */
class transaction_scope_error : public std::logic_error {
public:
	using std::logic_error::logic_error;
};

/**
 * Custom transaction error class.
 *
 * Thrown on manual transaction abort.
 * @ingroup exceptions
 */
class manual_tx_abort : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * Custom layout error class.
 *
 * Thrown when data layout is different than expected by the library.
 * @ingroup exceptions
 */
class layout_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * Custom ctl error class.
 *
 * Thrown on ctl_[get|set|exec] failure.
 * @ingroup exceptions
 */
class ctl_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

/**
 * Custom defrag error class.
 *
 * Thrown when the defragmentation process fails
 * (possibly in the middle of a run).
 * @ingroup exceptions
 */
class defrag_error : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;

	/**
	 * Construct error with partial results.
	 *
	 * @param result potential partial results of the defragmentation
	 * @param msg error message
	 */
	defrag_error(pobj_defrag_result result, const std::string &msg)
	    : std::runtime_error(msg), result(result)
	{
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
