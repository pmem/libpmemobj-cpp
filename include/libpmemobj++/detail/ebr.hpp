/*-
 * Copyright (c) 2015-2018 Mindaugas Rasiukevicius <rmind at noxt eu>
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

#ifndef LIBPMEMOBJ_EBR_HPP
#define LIBPMEMOBJ_EBR_HPP

#include <atomic>
#include <cassert>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace pmem
{

namespace detail
{

class ebr {
	using atomic = std::atomic<size_t>;
	using reference = std::reference_wrapper<atomic>;

public:
	class worker;

	ebr();

	worker register_worker();
	bool sync();
	size_t staging_epoch();
	size_t gc_epoch();

	class worker {
	public:
		worker(const worker &w) = delete;
		worker(worker &&w) = default;
		~worker();

		template <typename F>
		void critical(F &&f);

	private:
		worker(ebr *e_, reference ref);

		reference local_epoch;
		ebr *e;

		friend ebr;
	};

private:
	static const size_t ACTIVE_FLAG = static_cast<size_t>(1)
		<< (sizeof(size_t) * 8 - 1);

	atomic global_epoch;

	std::unordered_map<std::thread::id, atomic> workers;
	std::mutex mtx;
};

ebr::ebr()
{
	global_epoch.store(0);
}

ebr::worker
ebr::register_worker()
{
	mtx.lock();
	auto res = workers.emplace(std::this_thread::get_id(), 0);
	assert(res.second);
	mtx.unlock();

	return worker{this, reference{res.first->second}};
}

bool
ebr::sync()
{
	auto current_epoch = global_epoch.load();

	mtx.lock();
	for (auto &w : workers) {
		auto local_e = w.second.load();
		bool active = local_e & ACTIVE_FLAG;
		if (active && (local_e != (current_epoch | ACTIVE_FLAG))) {
			mtx.unlock();
			return false;
		}
	}
	mtx.unlock();

	global_epoch.store((current_epoch + 1) % 3);

	return true;
}

size_t
ebr::staging_epoch()
{
	return global_epoch.load();
}

size_t
ebr::gc_epoch()
{
	return (global_epoch.load() + 1) % 3;
}

ebr::worker::worker(ebr *e_, reference ref) : local_epoch(ref), e(e_)
{
}

ebr::worker::~worker()
{
	e->mtx.lock();
	e->workers.erase(std::this_thread::get_id());
	e->mtx.unlock();
}

template <typename F>
void
ebr::worker::critical(F &&f)
{
	auto new_epoch = e->global_epoch.load() | ACTIVE_FLAG;
	local_epoch.get().store(new_epoch);

	f();

	local_epoch.get().store(0);
}

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_EBR_HPP */
