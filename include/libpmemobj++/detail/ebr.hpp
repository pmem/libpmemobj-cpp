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

/**
 * @file
 * C++ EBR API.
 */

#ifndef LIBPMEMOBJ_EBR_HPP
#define LIBPMEMOBJ_EBR_HPP

#include <atomic>
#include <cassert>
#include <functional>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <libpmemobj++/detail/common.hpp>

namespace pmem
{

namespace detail
{

/**
 * Epoch-based reclamation (EBR). Reference:
 *
 *	K. Fraser, Practical lock-freedom,
 *	Technical Report UCAM-CL-TR-579, February 2004
 *	https://www.cl.cam.ac.uk/techreports/UCAM-CL-TR-579.pdf
 *
 * Summary:
 *
 * Any workers (threads or processes) actively referencing (accessing)
 * the globally visible objects must do that in the critical path covered
 * using the dedicated function. The grace period is determined using "epochs"
 * implemented as a global counter (and, for example, a dedicated G/C list for
 * each epoch). Objects in the current global epoch can be staged for
 * reclamation (garbage collection). Then, the objects in the target epoch can
 * be reclaimed after two successful increments of the global epoch. Only three
 * epochs are needed (e, e-1 and e-2), therefore we use clock arithmetics.
 */
class ebr {
	using atomic = std::atomic<size_t>;
	using reference = std::reference_wrapper<atomic>;

public:
	class worker;

	ebr();

	worker register_worker();
	bool sync();
	void full_sync();
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
	static const size_t EPOCHS_NUMBER = 3;

	atomic global_epoch;

	std::unordered_map<std::thread::id, atomic> workers;
	std::mutex mtx;
};

/**
 * Default and only ebr constructor.
 */
ebr::ebr() : global_epoch(0)
{
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	VALGRIND_HG_DISABLE_CHECKING(&global_epoch, sizeof(global_epoch));
#endif
}

/**
 * Registers and returns a new worker, which can perform critical operations
 * (accessing some shared data that can be removed in other threads). There can
 * be only one worker per thread. The worker will be automatically unregistered
 * in the destructor.
 *
 * @throw runtime_error if there is already a registered worker for the current
 * thread.
 *
 * @return new registered worker.
 */
ebr::worker
ebr::register_worker()
{
	std::lock_guard<std::mutex> lock(mtx);
	auto res = workers.emplace(std::this_thread::get_id(), 0);
	if (!res.second) {
		throw std::runtime_error(
			"There can be only one worker per thread");
	}

	return worker{this, reference{res.first->second}};
}

/**
 * Attempts to synchronise and announce a new epoch.
 *
 * The synchronisation points must be serialized (e.g. if there are multiple G/C
 * workers or other writers). Generally, calls to ebr::staging_epoch() and
 * ebr::gc_epoch() would be a part of the same serialized path (calling sync()
 * and gc_epoch()/staging_epoch() concurrently in two other threads will cause
 * an undefined behavior).
 *
 * @return true if a new epoch is announced and false if it wasn't possible in
 * the current state.
 */
bool
ebr::sync()
{
	auto current_epoch = global_epoch.load();

	std::lock_guard<std::mutex> lock(mtx);
	for (auto &w : workers) {
		auto local_e = w.second.load();
		bool active = local_e & ACTIVE_FLAG;
		if (active && (local_e != (current_epoch | ACTIVE_FLAG))) {
			return false;
		}
	}

	LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_BEFORE(std::memory_order_seq_cst,
					       &global_epoch);
	global_epoch.store((current_epoch + 1) % EPOCHS_NUMBER);

	return true;
}

/**
 * Perform full synchronisation ensuring that all objects which are no longer
 * globally visible (and potentially staged for reclamation) at the time of
 * calling this routine will be safe to reclaim/destroy after this
 * synchronisation routine completes and returns. Note: the synchronisation may
 * take across multiple epochs.
 */
void
ebr::full_sync()
{
	size_t syncs_cnt = 0;
	while (true) {
		if (sync() && ++syncs_cnt == EPOCHS_NUMBER) {
			break;
		}
	}
}

/**
 * Returns the epoch where objects can be staged for reclamation. This can be
 * used as a reference value for the pending queue/tag, used to postpone the
 * reclamation until this epoch becomes available for G/C. Note that this
 * function would normally be serialized together with the ebr::sync() calls.
 *
 * @return the epoch where objects can be staged for reclamation.
 */
size_t
ebr::staging_epoch()
{
	return global_epoch.load();
}

/**
 * Returns the epoch available for reclamation, i.e. the epoch where it is
 * guaranteed that the objects are safe to be reclaimed/destroyed. Note that
 * this function would normally be serialized together with the ebr::sync()
 * calls.
 *
 * @return the epoch available for reclamation.
 */
size_t
ebr::gc_epoch()
{
	return (global_epoch.load() + 1) % EPOCHS_NUMBER;
}

ebr::worker::worker(ebr *e_, reference ref) : local_epoch(ref), e(e_)
{
#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	VALGRIND_HG_DISABLE_CHECKING(&ref.get(), sizeof(ref.get()));
#endif
}

/**
 * Unregisters the worker from the list of the workers in the ebr.
 */
ebr::worker::~worker()
{
	std::lock_guard<std::mutex> lock(e->mtx);
	e->workers.erase(std::this_thread::get_id());
}

/**
 * Performs critical operations. Typically, this would be used by the readers
 * when accessing some shared data. Reclamation of objects is guaranteed not to
 * occur in the critical path.
 *
 * @param[in] f the function which will be executed as a critical operation.
 * This function's signature should be void().
 */
template <typename F>
void
ebr::worker::critical(F &&f)
{
	auto new_epoch = e->global_epoch.load() | ACTIVE_FLAG;
	LIBPMEMOBJ_CPP_ANNOTATE_HAPPENS_AFTER(std::memory_order_seq_cst,
					      &(e->global_epoch));

	local_epoch.get().store(new_epoch);

	f();

	local_epoch.get().store(0);
}

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_EBR_HPP */
