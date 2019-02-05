/*
 * Copyright 2019, Intel Corporation
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
