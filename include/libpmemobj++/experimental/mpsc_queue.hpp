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

/* XXX: Add documentation */
class mpsc_queue {
public:
	struct first_block {
		static constexpr size_t CAPACITY =
			CACHELINE_SIZE - sizeof(size_t);
		static constexpr size_t DIRTY_FLAG = (1ULL << 63);

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
				skip_consumed();
			}

			/* Invalidates data after increment */
			iterator &
			operator++()
			{
				auto pop = pmem::obj::pool_by_vptr(data);

				auto block =
					reinterpret_cast<first_block *>(data);

				assert(block->size != 0);

				auto element_end =
					reinterpret_cast<first_block *>(
						data +
						static_cast<ptrdiff_t>(
							block->size +
							sizeof(block->size)));

				/* Mark all cachelines as consumed. */
				pmem::obj::flat_transaction::run(pop, [&] {
					while (block < element_end) {
						block->size = 0;
						block++;
					}
				});

				/* Go to the next, unconsumed element. */
				skip_consumed();

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
			void
			skip_consumed()
			{
				auto b = reinterpret_cast<first_block *>(data);
				auto e = reinterpret_cast<first_block *>(end);

				/* Advance to first, unconsumed element */
				while (b < e) {
					if (b->size == 0) {
						b++;
					} else if (b->size &
						   size_t(first_block::
								  DIRTY_FLAG)) {
						auto size = b->size &
							(~size_t(
								first_block::
									DIRTY_FLAG));
						auto aligned_size =
							pmem::detail::align_up(
								size + sizeof(b->size),
								CACHELINE_SIZE);

						b += aligned_size /
							CACHELINE_SIZE;
					} else {
						break;
					}
				}

				assert(b <= e);

				this->data = reinterpret_cast<char *>(b);
			}

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

		/**
		 * @param f cannot fail. Any exception thrown from f will result
		 * in terminate().
		 */
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

			try {
				f(range);
			} catch (...) {
				std::terminate();
			}

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
			fblock.size =
				data.size() | size_t(first_block::DIRTY_FLAG);

			/*
			 * First step is to copy upto 56B of data and store
			 * data.size() with DIRTY flag set. After that, we store
			 * rest of the data in two steps:
			 *	1. Remainder of the data is aligned down to
			 *	cacheline and copied.
			 * Now, we are left with between 0 to 63 bytes. If
			 * nonzero:
			 *	2. Create a stack allocated cacheline-sized
			 *	buffer, fill in the remainder of the data, and
			 *	copy the entire cacheline. After all data is
			 *	store we clear the dirty flag from size.
			 *
			 * This is done so that we avoid a cache-miss on
			 * misaligned writes.
			 */

			size_t ncopy = (std::min)(
				data.size(), size_t(first_block::CAPACITY));
			std::copy_n(data.data(), ncopy, fblock.data);

			pmemobj_memcpy(queue->pop.handle(), log_data,
				       reinterpret_cast<char *>(&fblock),
				       CACHELINE_SIZE,
				       PMEMOBJ_F_MEM_NONTEMPORAL);

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

			pmemobj_drain(queue->pop.handle());

			fblock.size &= (~size_t(first_block::DIRTY_FLAG));

			pmemobj_memcpy(queue->pop.handle(), log_data,
				       reinterpret_cast<char *>(&fblock),
				       CACHELINE_SIZE,
				       PMEMOBJ_F_MEM_NONTEMPORAL);
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
	/** XXX: log should be zeroed out by the user. */
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
		buff_size_ =
			pmem::detail::align_down(buff_size_, CACHELINE_SIZE);
	}

	worker
	register_worker()
	{
		return worker(this);
	}

	/* XXX: hide wraparound behind iterators */
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
