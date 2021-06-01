/*
 * Copyright (c) 2016 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#ifndef RINGBUF_HPP
#define RINGBUF_HPP

#include <cstddef>

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <atomic>
#include <cassert>

#include <libpmemobj++/detail/atomic_backoff.hpp>

#ifdef _WIN32
#define __predict_false(x) (x)
#else
#define __predict_false(x) __builtin_expect((x) != 0, 0)
#endif /* _WIN32 */

namespace pmem
{

namespace obj
{

namespace experimental
{

namespace ringbuf
{
static constexpr size_t RBUF_OFF_MASK = 0x00000000ffffffffUL;
static constexpr size_t WRAP_LOCK_BIT = 0x8000000000000000UL;
static constexpr size_t RBUF_OFF_MAX = UINT64_MAX & ~WRAP_LOCK_BIT;

static constexpr size_t WRAP_COUNTER = 0x7fffffff00000000UL;
static size_t
WRAP_INCR(size_t x)
{
	return ((x + 0x100000000UL) & WRAP_COUNTER);
}

typedef uint64_t ringbuf_off_t;

typedef struct ringbuf_worker {
	std::atomic<ringbuf_off_t> seen_off;
	std::atomic<int> registered;
} ringbuf_worker_t;

typedef struct ringbuf {
	/* Ring buffer space. */
	size_t space;

	/*
	 * The NEXT hand is atomically updated by the producer.
	 * WRAP_LOCK_BIT is set in case of wrap-around; in such case,
	 * the producer can update the 'end' offset.
	 */
	std::atomic<ringbuf_off_t> next;
	std::atomic<ringbuf_off_t> end;

	/* The following are updated by the consumer. */
	std::atomic<ringbuf_off_t> written;
	unsigned nworkers;
	ringbuf_worker_t *workers;

	ringbuf(size_t max_workers, size_t length)
	{
		written.store(0);
		workers = new ringbuf_worker[max_workers];
		space = length;
		end = RBUF_OFF_MAX;
		nworkers = max_workers;
	}

	~ringbuf()
	{
		delete[] workers;
	}
} ringbuf_t;

/*
 * ringbuf_register: register the worker (thread/process) as a producer
 * and pass the pointer to its local store.
 */
inline ringbuf_worker_t *
ringbuf_register(ringbuf_t *rbuf, unsigned i)
{
	ringbuf_worker_t *w = &rbuf->workers[i];

	w->seen_off = RBUF_OFF_MAX;
	std::atomic_store_explicit<int>(&w->registered, true,
					std::memory_order_release);
	return w;
}

inline void
ringbuf_unregister(ringbuf_t *rbuf, ringbuf_worker_t *w)
{
	w->registered = false;
	(void)rbuf;
}

/*
 * stable_nextoff: capture and return a stable value of the 'next' offset.
 */
static inline ringbuf_off_t
stable_nextoff(ringbuf_t *rbuf)
{
	ringbuf_off_t next;
	for (pmem::detail::atomic_backoff backoff;;) {
		next = std::atomic_load_explicit<ringbuf_off_t>(
			&rbuf->next, std::memory_order_acquire);
		if (next & WRAP_LOCK_BIT) {
			backoff.pause();
		} else {
			break;
		}
	}
	assert((next & RBUF_OFF_MASK) < rbuf->space);
	return next;
}

/*
 * stable_seenoff: capture and return a stable value of the 'seen' offset.
 */
static inline ringbuf_off_t
stable_seenoff(ringbuf_worker_t *w)
{
	ringbuf_off_t seen_off;
	for (pmem::detail::atomic_backoff backoff;;) {
		seen_off = std::atomic_load_explicit<ringbuf_off_t>(
			&w->seen_off, std::memory_order_acquire);
		if (seen_off & WRAP_LOCK_BIT) {
			backoff.pause();
		} else {
			break;
		}
	}
	return seen_off;
}

/*
 * ringbuf_acquire: request a space of a given length in the ring buffer.
 *
 * => On success: returns the offset at which the space is available.
 * => On failure: returns -1.
 */
inline ssize_t
ringbuf_acquire(ringbuf_t *rbuf, ringbuf_worker_t *w, size_t len)
{
	ringbuf_off_t seen, next, target;

	assert(len > 0 && len <= rbuf->space);
	assert(w->seen_off == RBUF_OFF_MAX);

	do {
		ringbuf_off_t written;

		/*
		 * Get the stable 'next' offset.  Save the observed 'next'
		 * value (i.e. the 'seen' offset), but mark the value as
		 * unstable (set WRAP_LOCK_BIT).
		 *
		 * Note: CAS will issue a memory_order_release for us and
		 * thus ensures that it reaches global visibility together
		 * with new 'next'.
		 */
		seen = stable_nextoff(rbuf);
		next = seen & RBUF_OFF_MASK;
		assert(next < rbuf->space);
		std::atomic_store_explicit<ringbuf_off_t>(
			&w->seen_off, next | WRAP_LOCK_BIT,
			std::memory_order_relaxed);

		/*
		 * Compute the target offset.  Key invariant: we cannot
		 * go beyond the WRITTEN offset or catch up with it.
		 */
		target = next + len;
		written = rbuf->written;
		if (__predict_false(next < written && target >= written)) {
			/* The producer must wait. */
			std::atomic_store_explicit<ringbuf_off_t>(
				&w->seen_off, RBUF_OFF_MAX,
				std::memory_order_release);
			return -1;
		}

		if (__predict_false(target >= rbuf->space)) {
			const bool exceed = target > rbuf->space;

			/*
			 * Wrap-around and start from the beginning.
			 *
			 * If we would exceed the buffer, then attempt to
			 * acquire the WRAP_LOCK_BIT and use the space in
			 * the beginning.  If we used all space exactly to
			 * the end, then reset to 0.
			 *
			 * Check the invariant again.
			 */
			target = exceed ? (WRAP_LOCK_BIT | len) : 0;
			if ((target & RBUF_OFF_MASK) >= written) {
				std::atomic_store_explicit<ringbuf_off_t>(
					&w->seen_off, RBUF_OFF_MAX,
					std::memory_order_release);
				return -1;
			}
			/* Increment the wrap-around counter. */
			target |= WRAP_INCR(seen & WRAP_COUNTER);
		} else {
			/* Preserve the wrap-around counter. */
			target |= seen & WRAP_COUNTER;
		}
	} while (!std::atomic_compare_exchange_weak<ringbuf_off_t>(
		&rbuf->next, &seen, target));

	/*
	 * Acquired the range.  Clear WRAP_LOCK_BIT in the 'seen' value
	 * thus indicating that it is stable now.
	 *
	 * No need for memory_order_release, since CAS issued a fence.
	 */
	std::atomic_store_explicit<ringbuf_off_t>(&w->seen_off,
						  w->seen_off & ~WRAP_LOCK_BIT,
						  std::memory_order_relaxed);

	/*
	 * If we set the WRAP_LOCK_BIT in the 'next' (because we exceed
	 * the remaining space and need to wrap-around), then save the
	 * 'end' offset and release the lock.
	 */
	if (__predict_false(target & WRAP_LOCK_BIT)) {
		/* Cannot wrap-around again if consumer did not catch-up. */
		assert(rbuf->written <= next);
		assert(rbuf->end == RBUF_OFF_MAX);
		rbuf->end = next;
		next = 0;

		/*
		 * Unlock: ensure the 'end' offset reaches global
		 * visibility before the lock is released.
		 */
		std::atomic_store_explicit<ringbuf_off_t>(
			&rbuf->next, (target & ~WRAP_LOCK_BIT),
			std::memory_order_release);
	}
	assert((target & RBUF_OFF_MASK) <= rbuf->space);
	return (ssize_t)next;
}

/*
 * ringbuf_produce: indicate the acquired range in the buffer is produced
 * and is ready to be consumed.
 */
inline void
ringbuf_produce(ringbuf_t *rbuf, ringbuf_worker_t *w)
{
	(void)rbuf;
	assert(w->registered);
	assert(w->seen_off != RBUF_OFF_MAX);
	std::atomic_store_explicit<ringbuf_off_t>(&w->seen_off, RBUF_OFF_MAX,
						  std::memory_order_release);
}

/*
 * ringbuf_consume: get a contiguous range which is ready to be consumed.
 */
inline size_t
ringbuf_consume(ringbuf_t *rbuf, size_t *offset)
{
	ringbuf_off_t written = rbuf->written, next, ready;
	size_t towrite;
retry:
	/*
	 * Get the stable 'next' offset.  Note: stable_nextoff() issued
	 * a load memory barrier.  The area between the 'written' offset
	 * and the 'next' offset will be the *preliminary* target buffer
	 * area to be consumed.
	 */
	next = stable_nextoff(rbuf) & RBUF_OFF_MASK;
	if (written == next) {
		/* If producers did not advance, then nothing to do. */
		return 0;
	}

	/*
	 * Observe the 'ready' offset of each producer.
	 *
	 * At this point, some producer might have already triggered the
	 * wrap-around and some (or all) seen 'ready' values might be in
	 * the range between 0 and 'written'.  We have to skip them.
	 */
	ready = RBUF_OFF_MAX;

	for (unsigned i = 0; i < rbuf->nworkers; i++) {
		ringbuf_worker_t *w = &rbuf->workers[i];
		ringbuf_off_t seen_off;

		/*
		 * Skip if the worker has not registered.
		 *
		 * Get a stable 'seen' value.  This is necessary since we
		 * want to discard the stale 'seen' values.
		 */
		if (!std::atomic_load_explicit<int>(&w->registered,
						    std::memory_order_relaxed))
			continue;
		seen_off = stable_seenoff(w);

		/*
		 * Ignore the offsets after the possible wrap-around.
		 * We are interested in the smallest seen offset that is
		 * not behind the 'written' offset.
		 */
		if (seen_off >= written) {
			ready = std::min<ringbuf_off_t>(seen_off, ready);
		}
		assert(ready >= written);
	}

	/*
	 * Finally, we need to determine whether wrap-around occurred
	 * and deduct the safe 'ready' offset.
	 */
	if (next < written) {
		const ringbuf_off_t end =
			std::min<ringbuf_off_t>(rbuf->space, rbuf->end);

		/*
		 * Wrap-around case.  Check for the cut off first.
		 *
		 * Reset the 'written' offset if it reached the end of
		 * the buffer or the 'end' offset (if set by a producer).
		 * However, we must check that the producer is actually
		 * done (the observed 'ready' offsets are clear).
		 */
		if (ready == RBUF_OFF_MAX && written == end) {
			/*
			 * Clear the 'end' offset if was set.
			 */
			if (rbuf->end != RBUF_OFF_MAX) {
				rbuf->end = RBUF_OFF_MAX;
			}

			/*
			 * Wrap-around the consumer and start from zero.
			 */
			written = 0;
			std::atomic_store_explicit<ringbuf_off_t>(
				&rbuf->written, written,
				std::memory_order_release);
			goto retry;
		}

		/*
		 * We cannot wrap-around yet; there is data to consume at
		 * the end.  The ready range is smallest of the observed
		 * 'ready' or the 'end' offset.  If neither is set, then
		 * the actual end of the buffer.
		 */
		assert(ready > next);
		ready = std::min<ringbuf_off_t>(ready, end);
		assert(ready >= written);
	} else {
		/*
		 * Regular case.  Up to the observed 'ready' (if set)
		 * or the 'next' offset.
		 */
		ready = std::min<ringbuf_off_t>(ready, next);
	}
	towrite = ready - written;
	*offset = written;

	assert(ready >= written);
	assert(towrite <= rbuf->space);
	return towrite;
}

/*
 * ringbuf_release: indicate that the consumed range can now be released.
 */
inline void
ringbuf_release(ringbuf_t *rbuf, size_t nbytes)
{
	const size_t nwritten = rbuf->written + nbytes;

	assert(rbuf->written <= rbuf->space);
	assert(rbuf->written <= rbuf->end);
	assert(nwritten <= rbuf->space);

	rbuf->written = (nwritten == rbuf->space) ? 0 : nwritten;
}

} /* namespace ringbuf */
} /* namespace experimental */
} /* namespace obj*/
} /* namespace pmem*/

#endif /* RINGBUF_HPP */
