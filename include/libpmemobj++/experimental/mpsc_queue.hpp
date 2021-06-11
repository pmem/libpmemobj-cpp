// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#ifndef LIBPMEMOBJ_MPSC_QUEUE_HPP
#define LIBPMEMOBJ_MPSC_QUEUE_HPP

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/detail/ringbuf.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/slice.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <memory>

namespace pmem
{

namespace obj
{

namespace experimental
{

static constexpr size_t CACHELINE_SIZE = 64ULL;

/* XXX: Add documentation */
class mpsc_queue {
public:
	struct first_block {
		static constexpr size_t CAPACITY =
			CACHELINE_SIZE - sizeof(size_t);

		pmem::obj::p<size_t> size;
		char data[CAPACITY];
	};

	class read_accessor {
	private:
		char *data;
		size_t len;

		struct iterator {
			iterator(char *data, char *end) : data(data), end(end)
			{
				/* Advance to first, unconsumed element */
				while (this->data < end &&
				       reinterpret_cast<first_block *>(
					       this->data)
						       ->size == 0) {
					this->data += CACHELINE_SIZE;
				}
			}

			/* Invalidates data after increment */
			iterator &
			operator++()
			{
				auto pop = pmem::obj::pool_by_vptr(data);

				assert(reinterpret_cast<first_block *>(data)
					       ->size != 0);

				auto size =
					reinterpret_cast<first_block *>(data)
						->size;
				auto element_end = data +
					static_cast<ptrdiff_t>(size +
							       sizeof(size));

				assert(element_end <= end);

				/* Since after release, a user might produce
				 * smaller/longer value we must zero out first
				 * 8b of each cacheline within the value. */
				pmem::obj::flat_transaction::run(pop, [&] {
					while (data < element_end) {
						reinterpret_cast<first_block *>(
							data)
							->size = 0;
						data += CACHELINE_SIZE;
					}
				});

				while (data < end &&
				       reinterpret_cast<first_block *>(data)
						       ->size == 0) {
					data += CACHELINE_SIZE;
				}

				if (data >= end) {
					data = end;
					return *this;
				}

				return *this;
			}

			bool
			operator==(const iterator &rhs)
			{
				return data == rhs.data;
			}

			bool
			operator!=(const iterator &rhs)
			{
				return data != rhs.data;
			}

			pmem::obj::string_view operator*() const
			{
				auto b = reinterpret_cast<first_block *>(data);
				return pmem::obj::string_view(b->data, b->size);
			}

		private:
			char *data;
			char *end;
		};

	public:
		read_accessor(char *data, size_t len) : data(data), len(len)
		{
		}

		iterator
		begin()
		{
			return iterator(data, data + len);
		}

		iterator
		end()
		{
			return iterator(data + len, data + len);
		}
	};

	/* All workers should be destroyed before destruction of mpsc_queue */
	class worker {
	private:
		mpsc_queue *queue;
		ringbuf::ringbuf_worker_t *w;
		size_t id;

	public:
		worker(mpsc_queue *q)
		{
			queue = q;
			auto &manager = queue->get_id_manager();
			id = manager.get();
			w = ringbuf_register(queue->ring_buffer.get(), id);
		}

		worker(const worker &) = delete;

		worker &operator=(const worker &) = delete;

		worker(worker &&other)
		{
			*this = std::move(other);
		}

		worker &
		operator=(worker &&other)
		{
			if (this != &other) {
				queue = other.queue;
				w = other.w;
				id = other.id;

				other.queue = nullptr;
				other.w = nullptr;
			}
			return *this;
		}

		~worker()
		{
			if (w) {
				ringbuf_unregister(queue->ring_buffer.get(), w);
				auto &manager = queue->get_id_manager();
				manager.release(id);
			}
		}

		template <typename Function>
		bool
		try_produce(size_t size, Function &&f)
		{
			auto data = std::unique_ptr<char[]>(new char[size]);
			auto range = pmem::obj::slice<char *>(
				data.get(), data.get() + size);

			auto req_size = pmem::detail::align_up(
				size + sizeof(first_block::size),
				CACHELINE_SIZE);
			auto offset = ringbuf_acquire(queue->ring_buffer.get(),
						      w, req_size);

			if (offset == -1)
				return false;

			f(range);

			store_to_log(pmem::obj::string_view(data.get(), size),
				     queue->buf + offset);

			ringbuf_produce(queue->ring_buffer.get(), w);

			return true;
		}

		bool
		try_produce(pmem::obj::string_view data)
		{
			auto req_size = pmem::detail::align_up(
				data.size() + sizeof(first_block::size),
				CACHELINE_SIZE);
			auto offset = ringbuf_acquire(queue->ring_buffer.get(),
						      w, req_size);

			if (offset == -1)
				return false;

			store_to_log(data, queue->buf + offset);

			ringbuf_produce(queue->ring_buffer.get(), w);

			return true;
		}

	private:
		void
		store_to_log(pmem::obj::string_view data, char *log_data)
		{
			assert(reinterpret_cast<uintptr_t>(log_data) %
				       CACHELINE_SIZE ==
			       0);

			first_block fblock;
			fblock.size = 0;

			/*
			 * Depending on the size of the source data, we might
			 * need to perform up to three separate copies:
			 * 1. The first cacheline, 8b of metadata and 56b
			 *	of data If there's still data to be logged:
			 * 2. The entire remainder of data data aligned
			 *	down to cacheline, for example, if there's 150b
			 *	left, this step will copy only 128b. Now, we are
			 *	left with between 0 to 63 bytes. If nonzero:
			 * 3. Create a stack allocated cacheline-sized
			 *	buffer, fill in the remainder of the data, and
			 *	copy the entire cacheline.
			 *
			 * This is done so that we avoid a cache-miss on
			 * misaligned writes.
			 */

			size_t ncopy =
				(std::min)(data.size(), first_block::CAPACITY);
			std::copy_n(data.data(), ncopy, fblock.data);

			size_t remaining_size =
				ncopy > data.size() ? 0 : data.size() - ncopy;

			const char *srcof = data.data() + ncopy;
			size_t rcopy = pmem::detail::align_down(remaining_size,
								CACHELINE_SIZE);
			size_t lcopy = remaining_size - rcopy;

			char last_cacheline[CACHELINE_SIZE];
			if (lcopy != 0)
				std::copy_n(srcof + rcopy, lcopy,
					    last_cacheline);

			if (rcopy != 0) {
				char *dest = log_data + CACHELINE_SIZE;

				pmemobj_memcpy(
					queue->pop.handle(), dest, srcof, rcopy,
					PMEMOBJ_F_MEM_NODRAIN |
						PMEMOBJ_F_MEM_NONTEMPORAL);
			}

			if (lcopy != 0) {
				void *dest = log_data + CACHELINE_SIZE + rcopy;

				pmemobj_memcpy(
					queue->pop.handle(), dest,
					last_cacheline, CACHELINE_SIZE,
					PMEMOBJ_F_MEM_NODRAIN |
						PMEMOBJ_F_MEM_NONTEMPORAL);
			}

			pmemobj_memcpy(queue->pop.handle(), log_data,
				       reinterpret_cast<char *>(&fblock),
				       CACHELINE_SIZE,
				       PMEMOBJ_F_MEM_NODRAIN |
					       PMEMOBJ_F_MEM_NONTEMPORAL);

			pmemobj_drain(queue->pop.handle());

			auto b = reinterpret_cast<first_block *>(log_data);

			b->size = data.size();

			pmemobj_persist(queue->pop.handle(), &b->size,
					sizeof(b->size));
		}
	};

private:
	inline pmem::detail::id_manager &
	get_id_manager()
	{
		static pmem::detail::id_manager manager;
		return manager;
	}

	std::unique_ptr<ringbuf::ringbuf_t> ring_buffer;
	char *buf;
	pmem::obj::pool_base pop;
	size_t buff_size_;

public:
	mpsc_queue(pmem::obj::persistent_ptr<char[]> log, size_t buff_size,
		   size_t max_workers = 1)
	    : ring_buffer(new ringbuf::ringbuf_t(max_workers, buff_size))
	{
		pop = pmem::obj::pool_by_pptr(log);
		auto addr = (uintptr_t)log.get();
		auto aligned_addr =
			pmem::detail::align_up(addr, CACHELINE_SIZE);

		buf = (char *)aligned_addr;
		buff_size_ = buff_size - (aligned_addr - addr);
	}

	worker
	register_worker()
	{
		return worker(this);
	}

	template <typename Function>
	bool
	try_consume(Function &&f)
	{
		size_t offset;
		size_t len = ringbuf_consume(ring_buffer.get(), &offset);
		if (len != 0) {
			auto acc = read_accessor(buf + offset, len);
			f(acc);
			ringbuf_release(ring_buffer.get(), len);
			return true;
		}
		return false;
	}

	/* XXX - Move logic from this function to consume (this requires setting
	   reader/writer offsets in ringbuf) */
	template <typename Function>
	void
	recover(Function &&f)
	{
		auto acc = read_accessor(buf, buff_size_);
		auto it = acc.begin();
		while (it != acc.end()) {
			f(*it);
			++it;
		}
	}
};

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */

#endif /* LIBPMEMOBJ_MPSC_QUEUE_HPP */
