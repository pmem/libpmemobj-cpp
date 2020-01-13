/*
 * Copyright 2019-2020, Intel Corporation
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
 * Atomic backoff, for time delay.
 *
 * For a persistent version of concurrent hash map implementation
 * Ref: https://arxiv.org/abs/1509.02235

 */

#ifndef LIBPMEMOBJ_ATOMIC_BACKOFF_HPP
#define LIBPMEMOBJ_ATOMIC_BACKOFF_HPP

#include <thread>

#if _MSC_VER
#include <intrin.h>
#include <windows.h>
#endif

namespace pmem
{
namespace detail
{

class atomic_backoff {
	/**
	 * Time delay, in units of "pause" instructions.
	 * Should be equal to approximately the number of "pause" instructions
	 * that take the same time as context switch. Must be a power of two.
	 */
	static const int32_t LOOPS_BEFORE_YIELD = 16;
	int32_t count;

	static inline void
	__pause(int32_t delay)
	{
		for (; delay > 0; --delay) {
#if _MSC_VER
			YieldProcessor();
#elif __GNUC__ && (__i386__ || __x86_64__)
			// Only i386 and x86-64 have pause instruction
			__builtin_ia32_pause();
#endif
		}
	}

public:
	/**
	 * Deny copy constructor
	 */
	atomic_backoff(const atomic_backoff &) = delete;
	/**
	 * Deny assignment
	 */
	atomic_backoff &operator=(const atomic_backoff &) = delete;

	/** Default constructor */
	/* In many cases, an object of this type is initialized eagerly on hot
	 * path, as in for(atomic_backoff b; ; b.pause()) {...} For this reason,
	 * the construction cost must be very small! */
	atomic_backoff() : count(1)
	{
	}

	/**
	 * This constructor pauses immediately; do not use on hot paths!
	 */
	atomic_backoff(bool) : count(1)
	{
		pause();
	}

	/**
	 * Pause for a while.
	 */
	void
	pause()
	{
		if (count <= LOOPS_BEFORE_YIELD) {
			__pause(count);
			/* Pause twice as long the next time. */
			count *= 2;
		} else {
			/* Pause is so long that we might as well yield CPU to
			 * scheduler. */
			std::this_thread::yield();
		}
	}

	/**
	 * Pause for a few times and return false if saturated.
	 */
	bool
	bounded_pause()
	{
		__pause(count);
		if (count < LOOPS_BEFORE_YIELD) {
			/* Pause twice as long the next time. */
			count *= 2;
			return true;
		} else {
			return false;
		}
	}

	void
	reset()
	{
		count = 1;
	}
}; /* class atomic_backoff */

} /* namespace detail */

} /* namespace pmem */

#endif
