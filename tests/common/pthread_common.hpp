// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2018, Intel Corporation */
#ifndef PTHREAD_COMMON_HPP
#define PTHREAD_COMMON_HPP

#include "unittest.hpp"
#include <pthread.h>

void
ut_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
		  void *(*start_routine)(void *), void *arg)
{
	if (pthread_create(thread, attr, start_routine, arg) != 0)
		UT_FATAL("pthread_create failed");
}

void
ut_pthread_join(pthread_t *thread, void **value)
{
	if (pthread_join(*thread, value) != 0)
		UT_FATAL("pthread_join failed");
}

#endif /* PTHREAD_COMMON_HPP */
