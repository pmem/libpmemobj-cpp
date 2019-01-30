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

#include "valgrind_internal.hpp"
#include <libpmemobj++/detail/common.hpp>

unsigned On_valgrind = 0;
unsigned On_pmemcheck = 0;
unsigned On_memcheck = 0;
unsigned On_helgrind = 0;
unsigned On_drd = 0;

void
set_valgrind_internals(void)
{
#if LIBPMEMOBJ_CPP_ANY_VG_TOOL_ENABLED
	On_valgrind = RUNNING_ON_VALGRIND;

	if (On_valgrind) {
		if (getenv("LIBPMEMOBJ_CPP_TRACER_PMEMCHECK"))
			On_pmemcheck = 1;
		else if (getenv("LIBPMEMOBJ_CPP_TRACER_MEMCHECK"))
			On_memcheck = 1;
		else if (getenv("LIBPMEMOBJ_CPP_TRACER_HELGRIND"))
			On_helgrind = 1;
		else if (getenv("LIBPMEMOBJ_CPP_TRACER_DRD"))
			On_drd = 1;
	}
#endif /* LIBPMEMOBJ_CPP_ANY_VG_TOOL_ENABLED */
}
