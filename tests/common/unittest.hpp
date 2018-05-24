/*
 * Copyright 2018, Intel Corporation
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

#ifndef LIBPMEMOBJ_CPP_UNITTEST_HPP
#define LIBPMEMOBJ_CPP_UNITTEST_HPP

#include <cstdio>
#include <cstdlib>
#include <string>

#ifndef _WIN32
typedef struct stat os_stat_t;
#define os_stat stat
#else
typedef struct _stat64 os_stat_t;
#define os_stat _stat64
#endif

template<typename ...Args>
static inline void
UT_OUT(Args ...args)
{
    fprintf(stdout, args...);
    fprintf(stdout, "\n");
}

template<typename ...Args>
static inline void
UT_FATAL(Args ...args)
{
    fprintf(stderr, args...);
    fprintf(stderr, "\n");
    abort();
}

/* assert a condition is true at runtime */
#define UT_ASSERT(cnd)\
	((void)((cnd) || (UT_FATAL(\
	"%s:%d %s - assertion failure: %s",\
	__FILE__, __LINE__, __func__, #cnd), 0)))

/* assertion with extra info printed if assertion fails at runtime */
#define UT_ASSERTinfo(cnd, info) \
	((void)((cnd) || (UT_FATAL(\
	"%s:%d %s - assertion failure: %s (%s = %s)",\
	__FILE__, __LINE__, __func__, #cnd, #info, info), 0)))

/* assert two integer values are equal at runtime */
#define UT_ASSERTeq(lhs, rhs)\
	((void)(((lhs) == (rhs)) || (UT_FATAL(\
	"%s:%d %s - assertion failure: %s (0x%llx) == %s (0x%llx)",\
	__FILE__, __LINE__, __func__, #lhs,\
	(unsigned long long)(lhs), #rhs, (unsigned long long)(rhs)), 0)))

/* assert two integer values are not equal at runtime */
#define UT_ASSERTne(lhs, rhs)\
	((void)(((lhs) != (rhs)) || (UT_FATAL(\
	"%s:%d %s - assertion failure: %s (0x%llx) != %s (0x%llx)",\
	__FILE__, __LINE__, __func__, #lhs,\
	(unsigned long long)(lhs), #rhs, (unsigned long long)(rhs)), 0)))

#endif /* LIBPMEMOBJ_CPP_UNITTEST_HPP */
