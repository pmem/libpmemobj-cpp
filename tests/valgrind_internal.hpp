// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

#ifndef VALGRIND_INTERNAL_HPP
#define VALGRIND_INTERNAL_HPP

extern unsigned On_valgrind;
extern unsigned On_pmemcheck;
extern unsigned On_memcheck;
extern unsigned On_helgrind;
extern unsigned On_drd;

#if LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED

#define VALGRIND_ADD_TO_TX(addr, len)                                          \
	do {                                                                   \
		if (On_pmemcheck)                                              \
			VALGRIND_PMC_ADD_TO_TX(addr, len);                     \
	} while (0)

#define VALGRIND_SET_CLEAN(addr, len)                                          \
	do {                                                                   \
		if (On_pmemcheck)                                              \
			VALGRIND_PMC_SET_CLEAN(addr, len);                     \
	} while (0)

#define VALGRIND_REMOVE_FROM_TX(addr, len)                                     \
	do {                                                                   \
		if (On_pmemcheck)                                              \
			VALGRIND_PMC_REMOVE_FROM_TX(addr, len);                \
	} while (0)

#else

#define VALGRIND_ADD_TO_TX(addr, len)                                          \
	do {                                                                   \
		(void)(addr);                                                  \
		(void)(len);                                                   \
	} while (0)

#define VALGRIND_SET_CLEAN(addr, len)                                          \
	do {                                                                   \
		(void)(addr);                                                  \
		(void)(len);                                                   \
	} while (0)

#define VALGRIND_REMOVE_FROM_TX(addr, len)                                     \
	do {                                                                   \
		(void)(addr);                                                  \
		(void)(len);                                                   \
	} while (0)

#endif /* LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED */

void set_valgrind_internals(void);

#endif /* VALGRIND_INTERNAL_HPP */
