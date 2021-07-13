// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2021, Intel Corporation */

/**
 * @file
 * Commonly used functionality.
 */

#ifndef LIBPMEMOBJ_CPP_COMMON_HPP
#define LIBPMEMOBJ_CPP_COMMON_HPP

#include <libpmemobj++/pexceptions.hpp>
#include <libpmemobj/tx_base.h>
#include <string>
#include <typeinfo>

#if _MSC_VER
#include <intrin.h>
#include <windows.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
#define POBJ_CPP_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define POBJ_CPP_DEPRECATED __declspec(deprecated)
#else
#define POBJ_CPP_DEPRECATED
#endif

#if LIBPMEMOBJ_CPP_VG_ENABLED
#undef LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED
#undef LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED
#undef LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
#undef LIBPMEMOBJ_CPP_VG_DRD_ENABLED

#define LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED 1
#define LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED 1
#define LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED 1
#define LIBPMEMOBJ_CPP_VG_DRD_ENABLED 1
#endif

#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED ||                                     \
	LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED ||                                  \
	LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED || LIBPMEMOBJ_CPP_VG_DRD_ENABLED
#define LIBPMEMOBJ_CPP_ANY_VG_TOOL_ENABLED 1
#endif

#if LIBPMEMOBJ_CPP_ANY_VG_TOOL_ENABLED
#include <valgrind.h>
#endif

#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED
#include <pmemcheck.h>
#endif

#if LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED
#include <memcheck.h>
#endif

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
#include <helgrind.h>
#endif

#if LIBPMEMOBJ_CPP_VG_DRD_ENABLED
#include <drd.h>
#endif

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED

#define LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(order, ptr)                     \
	if (order == std::memory_order_release ||                              \
	    order == std::memory_order_acq_rel ||                              \
	    order == std::memory_order_seq_cst) {                              \
		ANNOTATE_HAPPENS_BEFORE(ptr);                                  \
	}

#define LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(order, ptr)                      \
	if (order == std::memory_order_consume ||                              \
	    order == std::memory_order_acquire ||                              \
	    order == std::memory_order_acq_rel ||                              \
	    order == std::memory_order_seq_cst) {                              \
		ANNOTATE_HAPPENS_AFTER(ptr);                                   \
	}
#else

#define LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(order, ptr)
#define LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(order, ptr)

#endif

/*
 * Workaround for missing "is_trivially_copyable" in gcc < 5.0.
 * Be aware of a difference between __has_trivial_copy and is_trivially_copyable
 * e.g. for deleted copy constructors __has_trivial_copy(A) returns 1 in clang
 * and 0 in gcc. It means that for gcc < 5 LIBPMEMOBJ_CPP_IS_TRIVIALLY_COPYABLE
 * is more restrictive than is_trivially_copyable.
 */
#if !defined(LIBPMEMOBJ_CPP_USE_HAS_TRIVIAL_COPY)
#if !defined(__clang__) && defined(__GNUG__) && __GNUC__ < 5
#define LIBPMEMOBJ_CPP_USE_HAS_TRIVIAL_COPY 1
#else
#define LIBPMEMOBJ_CPP_USE_HAS_TRIVIAL_COPY 0
#endif
#endif

#if LIBPMEMOBJ_CPP_USE_HAS_TRIVIAL_COPY
#define LIBPMEMOBJ_CPP_IS_TRIVIALLY_COPYABLE(T) __has_trivial_copy(T)
#else
#define LIBPMEMOBJ_CPP_IS_TRIVIALLY_COPYABLE(T)                                \
	std::is_trivially_copyable<T>::value
#endif

/*! \namespace pmem
 * \brief Persistent memory namespace.
 *
 * It is a common namespace for all persistent memory C++ libraries
 * For more information about pmem goto: https://pmem.io
 */
namespace pmem
{
/*! \namespace pmem::obj
 * \brief Main libpmemobj namespace.
 *
 * It contains all libpmemobj's public types, enums, classes with their
 * functions and members. It is located within pmem namespace.
 */
namespace obj
{
template <typename T>
class persistent_ptr;

/*! \namespace pmem::obj::experimental
 * \brief Experimental implementations.
 *
 * It contains implementations, which are not yet ready to be used in
 * production. They may be not finished, not fully tested or still in
 * discussion. It is located within pmem::obj namespace.
 */
namespace experimental
{
}
}

/*! \namespace pmem::detail
 * \brief Implementation details.
 *
 * It contains libpmemobj's implementation details, not needed in public
 * headers. It is located within pmem namespace.
 */
namespace detail
{

#if defined(__x86_64) || defined(_M_X64) || defined(__aarch64__) ||            \
	defined(__riscv)
static constexpr size_t CACHELINE_SIZE = 64ULL;
#elif defined(__PPC64__)
static constexpr size_t CACHELINE_SIZE = 128ULL;
#else
#error unable to recognize architecture at compile time
#endif

template <class T1 /* in case of ENOMEM */, class T2 /* otherwise */>
void
throw_when_errno(std::string exception_msg)
{
	if (errno == ENOMEM) {
		throw T1(exception_msg).with_pmemobj_errormsg();
	} else {
		throw T2(exception_msg).with_pmemobj_errormsg();
	}
}

/*
 * Conditionally add 'count' objects to a transaction.
 *
 * Adds count objects starting from `that` to the transaction if '*that' is
 * within a pmemobj pool and there is an active transaction.
 * Does nothing otherwise.
 *
 * @param[in] that pointer to the first object being added to the transaction.
 * @param[in] count number of elements to be added to the transaction.
 * @param[in] flags is a bitmask of values which are described in libpmemobj
 * manpage (pmemobj_tx_xadd_range method)
 */
template <typename T>
inline void
conditional_add_to_tx(const T *that, std::size_t count = 1, uint64_t flags = 0)
{
	if (count == 0)
		return;

	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		return;

	/* 'that' is not in any open pool */
	if (!pmemobj_pool_by_ptr(that))
		return;

	if (pmemobj_tx_xadd_range_direct(that, sizeof(*that) * count, flags)) {
		throw_when_errno<pmem::transaction_out_of_memory,
				 pmem::transaction_error>(
			"Could not add object(s) to the transaction.");
	}
}

/*
 * Return type number for given type.
 */
template <typename T>
uint64_t
type_num()
{
	return typeid(T).hash_code();
}

/**
 * Round up to the next lowest power of 2. Overload for uint64_t argument.
 */
inline uint64_t
next_pow_2(uint64_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v |= v >> 32;
	++v;
	return v + (v == 0);
}

/**
 * Round up to the next lowest power of 2. Overload for uint32_t argument.
 */
inline uint64_t
next_pow_2(uint32_t v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	++v;
	return v + (v == 0);
}

#if _MSC_VER
static inline int
Log2(uint64_t x)
{
	unsigned long j;
	_BitScanReverse64(&j, x);
	return static_cast<int>(j);
}
#elif __GNUC__ || __clang__
static inline int
Log2(uint64_t x)
{
	// __builtin_clz builtin count _number_ of leading zeroes
	return 8 * int(sizeof(x)) - __builtin_clzll(x) - 1;
}
#else
static inline int
Log2(uint64_t x)
{
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);
	x |= (x >> 32);

	static const int table[64] = {
		0,  58, 1,  59, 47, 53, 2,  60, 39, 48, 27, 54, 33, 42, 3,  61,
		51, 37, 40, 49, 18, 28, 20, 55, 30, 34, 11, 43, 14, 22, 4,  62,
		57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56,
		45, 25, 31, 35, 16, 9,	12, 44, 24, 15, 8,  23, 7,  6,	5,  63};

	return table[(x * 0x03f6eaf2cd271461) >> 58];
}
#endif

#ifndef _MSC_VER

/** Returns index of most significant set bit */
static inline uint8_t
mssb_index64(unsigned long long value)
{
	return ((uint8_t)(63 - __builtin_clzll(value)));
}

/** Returns index of most significant set bit */
static inline uint8_t
mssb_index(unsigned int value)
{
	return ((uint8_t)(31 - __builtin_clz(value)));
}

#else

static __inline uint8_t
mssb_index(unsigned long value)
{
	unsigned long ret;
	_BitScanReverse(&ret, value);
	return (uint8_t)ret;
}

static __inline uint8_t
mssb_index64(uint64_t value)
{
	unsigned long ret;
	_BitScanReverse64(&ret, value);
	return (uint8_t)ret;
}

#endif

static constexpr size_t
align_up(size_t size, size_t align)
{
	return ((size) + (align)-1) & ~((align)-1);
}

static constexpr size_t
align_down(size_t size, size_t align)
{
	return (size) & ~((align)-1);
}

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_COMMON_HPP */
