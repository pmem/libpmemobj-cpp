// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019, Intel Corporation */

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
