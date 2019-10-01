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

/**
 * @file
 * C++ ctl api.
 */

#ifndef LIBPMEMOBJ_CPP_CTL_HPP
#define LIBPMEMOBJ_CPP_CTL_HPP

#include <libpmemobj/ctl.h>
#include <string>

#include <cerrno>
#include <cstring>

#include <libpmemobj++/pexceptions.hpp>

namespace pmem
{

namespace obj
{

template <typename T>
T
ctl_get_detail(PMEMobjpool *pool, const std::string &name)
{
	T tmp;

#ifdef _WIN32
	int ret = pmemobj_ctl_getU(pool, name.c_str(), &tmp);
#else
	int ret = pmemobj_ctl_get(pool, name.c_str(), &tmp);
#endif
	if (ret)
		throw pmem::ctl_error("ctl_get failed").with_pmemobj_errormsg();

	return tmp;
}

template <typename T>
T
ctl_set_detail(PMEMobjpool *pool, const std::string &name, T arg)
{
#ifdef _WIN32
	int ret = pmemobj_ctl_setU(pool, name.c_str(), &arg);
#else
	int ret = pmemobj_ctl_set(pool, name.c_str(), &arg);
#endif
	if (ret)
		throw pmem::ctl_error("ctl_set failed").with_pmemobj_errormsg();

	return arg;
}

template <typename T>
T
ctl_exec_detail(PMEMobjpool *pool, const std::string &name, T arg)
{
#ifdef _WIN32
	int ret = pmemobj_ctl_execU(pool, name.c_str(), &arg);
#else
	int ret = pmemobj_ctl_exec(pool, name.c_str(), &arg);
#endif
	if (ret)
		throw pmem::ctl_error("ctl_exec failed")
			.with_pmemobj_errormsg();

	return arg;
}

#ifdef _WIN32
template <typename T>
T
ctl_get_detail(PMEMobjpool *pool, const std::wstring &name)
{
	T tmp;

	int ret = pmemobj_ctl_getW(pool, name.c_str(), &tmp);
	if (ret)
		throw pmem::ctl_error("ctl_get failed").with_pmemobj_errormsg();

	return tmp;
}

template <typename T>
T
ctl_set_detail(PMEMobjpool *pool, const std::wstring &name, T arg)
{
	int ret = pmemobj_ctl_setW(pool, name.c_str(), &arg);
	if (ret)
		throw pmem::ctl_error("ctl_set failed").with_pmemobj_errormsg();

	return arg;
}

template <typename T>
T
ctl_exec_detail(PMEMobjpool *pool, const std::wstring &name, T arg)
{
	int ret = pmemobj_ctl_execW(pool, name.c_str(), &arg);
	if (ret)
		throw pmem::ctl_error("ctl_exec failed")
			.with_pmemobj_errormsg();

	return arg;
}
#endif

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_CTL_HPP */
