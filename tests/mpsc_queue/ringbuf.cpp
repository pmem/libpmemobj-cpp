/*-
 * Copyright (c) 2016-2017 Mindaugas Rasiukevicius <rmind at noxt eu>
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

/*
 * ringbuf.cpp -- tests for internal implementaiton of ringbuffer used in
 * pmem::obj::experimental::mpsc_queue
 */

#include "unittest.hpp"

#include <random>

#include <libpmemobj++/detail/ringbuf.hpp>

static std::mt19937_64 generator;

using namespace pmem::obj::experimental::ringbuf;

#define MAX_WORKERS 2

static void
test_wraparound(void)
{
	const size_t n = 1000;
	ringbuf_t *r = new ringbuf_t(MAX_WORKERS, n);
	ringbuf_worker_t *w;
	size_t len, woff;
	ptrdiff_t off;

	/* Size n, but only (n - 1) can be produced at a time. */
	w = ringbuf_register(r, 0);

	/* Produce (n / 2 + 1) and then attempt another (n / 2 - 1). */
	off = ringbuf_acquire(r, w, n / 2 + 1);
	UT_ASSERT(off == 0);
	ringbuf_produce(r, w);

	off = ringbuf_acquire(r, w, n / 2 - 1);
	UT_ASSERT(off == -1);

	/* Consume (n / 2 + 1) bytes. */
	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == (n / 2 + 1) && woff == 0);
	ringbuf_release(r, len);

	/* All consumed, attempt (n / 2 + 1) now. */
	off = ringbuf_acquire(r, w, n / 2 + 1);
	UT_ASSERT(off == -1);

	/* However, wraparound can be successful with (n / 2). */
	off = ringbuf_acquire(r, w, n / 2);
	UT_ASSERT(off == 0);
	ringbuf_produce(r, w);

	/* Consume (n / 2) bytes. */
	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == (n / 2) && woff == 0);
	ringbuf_release(r, len);

	ringbuf_unregister(r, w);
	delete r;
}

static void
test_multi(void)
{

	ringbuf_t *r = new ringbuf_t(MAX_WORKERS, 3);
	ringbuf_worker_t *w;
	size_t len, woff;
	ptrdiff_t off;

	w = ringbuf_register(r, 0);

	/*
	 * Produce 2 bytes.
	 */

	off = ringbuf_acquire(r, w, 1);
	UT_ASSERT(off == 0);
	ringbuf_produce(r, w);

	off = ringbuf_acquire(r, w, 1);
	UT_ASSERT(off == 1);
	ringbuf_produce(r, w);

	off = ringbuf_acquire(r, w, 1);
	UT_ASSERT(off == -1);

	/*
	 * Consume 2 bytes.
	 */
	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 2 && woff == 0);
	ringbuf_release(r, len);

	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 0);

	/*
	 * Produce another 2 with wrap-around.
	 */

	off = ringbuf_acquire(r, w, 2);
	UT_ASSERT(off == -1);

	off = ringbuf_acquire(r, w, 1);
	UT_ASSERT(off == 2);
	ringbuf_produce(r, w);

	off = ringbuf_acquire(r, w, 1);
	UT_ASSERT(off == 0);
	ringbuf_produce(r, w);

	off = ringbuf_acquire(r, w, 1);
	UT_ASSERT(off == -1);

	/*
	 * Consume 1 byte at the end and 1 byte at the beginning.
	 */

	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 1 && woff == 2);
	ringbuf_release(r, len);

	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 1 && woff == 0);
	ringbuf_release(r, len);

	ringbuf_unregister(r, w);
	delete r;
}

static void
test_overlap(void)
{
	ringbuf_t *r = new ringbuf_t(MAX_WORKERS, 10);
	ringbuf_worker_t *w1, *w2;
	size_t len, woff;
	ptrdiff_t off;

	w1 = ringbuf_register(r, 0);
	w2 = ringbuf_register(r, 1);

	/*
	 * Producer 1: acquire 5 bytes.  Consumer should fail.
	 */
	off = ringbuf_acquire(r, w1, 5);
	UT_ASSERT(off == 0);

	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 0);

	/*
	 * Producer 2: acquire 3 bytes.  Consumer should still fail.
	 */
	off = ringbuf_acquire(r, w2, 3);
	UT_ASSERT(off == 5);

	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 0);

	/*
	 * Producer 1: commit.  Consumer can get the first range.
	 */
	ringbuf_produce(r, w1);
	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 5 && woff == 0);
	ringbuf_release(r, len);

	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 0);

	/*
	 * Producer 1: acquire-produce 4 bytes, triggering wrap-around.
	 * Consumer should still fail.
	 */
	off = ringbuf_acquire(r, w1, 4);
	UT_ASSERT(off == 0);

	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 0);

	ringbuf_produce(r, w1);
	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 0);

	/*
	 * Finally, producer 2 commits its 3 bytes.
	 * Consumer can proceed for both ranges.
	 */
	ringbuf_produce(r, w2);
	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 3 && woff == 5);
	ringbuf_release(r, len);

	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 4 && woff == 0);
	ringbuf_release(r, len);

	/*
	 * Previous consumer consumed all data, so next one should fail
	 */
	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 0);

	ringbuf_unregister(r, w1);
	ringbuf_unregister(r, w2);

	/*
	 * After unregistration of all producers, consumer should still fail
	 */
	len = ringbuf_consume(r, &woff);
	UT_ASSERT(len == 0);

	delete r;
}

static void
test_random(void)
{
	ptrdiff_t off1 = -1, off2 = -1;
	unsigned n = 1000 * 1000 * 50;
	constexpr size_t buff_size = 500;
	unsigned char buf[buff_size];

	ringbuf_t *r = new ringbuf_t(MAX_WORKERS, buff_size);
	ringbuf_worker_t *w1, *w2;

	w1 = ringbuf_register(r, 0);
	w2 = ringbuf_register(r, 1);

	while (n--) {
		size_t len, woff;

		len = static_cast<size_t>(generator()) % (sizeof(buf) / 2) + 1;
		switch (generator() % 3) {
			/* consumer */
			case 0:
				len = ringbuf_consume(r, &woff);
				if (len > 0) {
					size_t vlen = 0;
					UT_ASSERT(woff < sizeof(buf));
					while (vlen < len) {
						size_t mlen =
							(unsigned)buf[woff];
						UT_ASSERT(mlen > 0);
						vlen += mlen;
						woff += mlen;
					}
					UT_ASSERT(vlen == len);
					ringbuf_release(r, len);
				}
				break;
				/* producer 1 */
			case 1:
				if (off1 == -1) {
					if ((off1 = ringbuf_acquire(
						     r, w1, len)) >= 0) {
						UT_ASSERT((size_t)off1 <
							  sizeof(buf));
						buf[off1] = len - 1;
					}
				} else {
					buf[off1]++;
					ringbuf_produce(r, w1);
					off1 = -1;
				}
				break;
			/* producer 2 */
			case 2:
				if (off2 == -1) {
					if ((off2 = ringbuf_acquire(
						     r, w2, len)) >= 0) {
						UT_ASSERT((size_t)off2 <
							  sizeof(buf));
						buf[off2] = len - 1;
					}
				} else {
					buf[off2]++;
					ringbuf_produce(r, w2);
					off2 = -1;
				}
				break;
		}
	}
	ringbuf_unregister(r, w1);
	ringbuf_unregister(r, w2);
	delete r;
}

static void
test_size()
{
	try {
		auto size = (1ULL << 32) + 1;
		auto r = new ringbuf_t(1, size);
		(void)r;

		ASSERT_UNREACHABLE;
	} catch (std::out_of_range &) {

	} catch (...) {
		ASSERT_UNREACHABLE;
	}
}

int
main(void)
{
	std::random_device rd;
	auto seed = rd();
	std::cout << "rand seed: " << seed << std::endl;
	generator = std::mt19937_64(seed);

	test_wraparound();
	test_multi();
	test_overlap();
	test_random();
	test_size();
	return 0;
}
