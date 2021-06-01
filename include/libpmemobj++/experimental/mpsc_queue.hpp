// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

#ifndef LIBPMEMOBJ_MPSC_QUEUE_HPP
#define LIBPMEMOBJ_MPSC_QUEUE_HPP

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

// XXX: Move id_manager to separate file
#include <iterator>
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/slice.hpp>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <libpmemobj++/detail/ringbuf.hpp>
#include <memory>
#include <stdexcept>

namespace pmem
{

namespace obj
{

namespace experimental
{

static constexpr size_t CACHELINE_SIZE = 64ULL;

class mpsc_queue {
public:
	struct entry {
		char data[56];
		size_t size;
	};

	class read_accessor {
	private:
		char *data;
		size_t len;

		struct iterator {
			iterator(char *data) : data(data)
			{
			}

			// Invalidates data after increment
			iterator &
			operator++()
			{
				auto pop = pmem::obj::pool_by_vptr(data);
				entry *entry_ptr = (entry *)data;

				entry_ptr->size = 0;
				pop.persist(&entry_ptr->size,
					    sizeof(entry_ptr->size));

				data += CACHELINE_SIZE;

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
				entry *entry_ptr = (entry *)data;
				return pmem::obj::string_view(entry_ptr->data,
							      entry_ptr->size);
			}

		private:
			char *data;
		};

	public:
		read_accessor(char *data, size_t len) : data(data), len(len)
		{
		}

		iterator
		begin()
		{
			return iterator(data);
		}

		iterator
		end()
		{
			return iterator(data + len);
		}
	};

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

		~worker()
		{
			ringbuf_unregister(queue->ring_buffer.get(), w);
			auto &manager = queue->get_id_manager();
			manager.release(id);
		}

		template <typename Function>
		bool
		try_produce(size_t size, Function &&f)
		{
			entry dram_entry;
			size_t aligned_len = ALIGN_UP(size, CACHELINE_SIZE);
			assert(aligned_len % CACHELINE_SIZE == 0);
			auto offset = ringbuf_acquire(queue->ring_buffer.get(),
						      w, aligned_len);
			if (offset != -1) {
				char *data = queue->buf + offset;
				auto range = pmem::obj::slice<char *>(
					dram_entry.data,
					dram_entry.data + size);
				f(range);
				dram_entry.size = size;
				pmemobj_memcpy(queue->pop.handle(), data,
					       (char *)&dram_entry,
					       CACHELINE_SIZE,
					       PMEMOBJ_F_MEM_NONTEMPORAL);
				pmemobj_drain(queue->pop.handle());

				ringbuf_produce(queue->ring_buffer.get(), w);
				return true;
			}
			return false;
		}
	};

private:
	inline pmem::detail::id_manager &
	get_id_manager()
	{
		static pmem::detail::id_manager manager;
		return manager;
	}

	static size_t
	ALIGN_UP(size_t size, size_t align)
	{
		return (((size) + (align)-1) & ~((align)-1));
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
		auto aligned_addr = ALIGN_UP(addr, CACHELINE_SIZE);

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

	// XXX - Move logic from this function to consume (this requires setting
	// reader/writer offsets int ringubf)
	template <typename F>
	void
	recover(F &&f)
	{
		for (size_t i = 0; i < buff_size_; i += CACHELINE_SIZE) {
			entry dram_entry;
			memcpy((char *)&dram_entry, buf + i, CACHELINE_SIZE);

			if (dram_entry.size)
				f(dram_entry);
		}
	}
};

}
}
}

#endif /* LIBPMEMOBJ_MPSC_QUEUE_HPP */
