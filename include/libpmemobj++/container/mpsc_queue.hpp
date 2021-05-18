#ifndef LIBPMEMOBJ_MPSC_QUEUE_HPP
#define LIBPMEMOBJ_MPSC_QUEUE_HPP

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>

// XXX: Move id_manager to separate file
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/slice.hpp>
#include <iterator>

#include <atomic>
#include <cstddef>
#include <cstring>
#include <libpmemobj++/container/detail/ringbuf.h>

namespace pmem
{

namespace obj
{

namespace experimental
{

static constexpr size_t CACHELINE_SIZE = 64ULL;
#define ALIGN_UP(size, align) (((size) + (align)-1) & ~((align)-1))

class mpsc_queue {
public:
	struct entry {
		char data[56];
		size_t size;
	};

	class accessor {
	private:
		entry dram_entry;

		ringbuf::ringbuf_worker_t *w;
		mpsc_queue *queue;

		char *data;

	public:
		accessor(mpsc_queue *q, ringbuf::ringbuf_worker_t *worker,
			 size_t len)
		{
			assert(len <= CACHELINE_SIZE && len > 0);
			dram_entry.size = len;
			size_t aligned_len = ALIGN_UP(len, CACHELINE_SIZE);

			w = worker;
			queue = q;
			auto offset =
				ringbuf_acquire(queue->ring_buffer, w, aligned_len);
			data = queue->buf + offset;
		};

		~accessor()
		{
			//dram_entry.size = size;
			pmemobj_memcpy(queue->pop.handle(), data,
				       (char *)&dram_entry, CACHELINE_SIZE,
				       PMEMOBJ_F_MEM_NONTEMPORAL);
			pmemobj_drain(queue->pop.handle());

			ringbuf_produce(queue->ring_buffer, w);
		}

		pmem::obj::slice<char*> get_range() {
			return pmem::obj::slice<char*>(dram_entry.data, dram_entry.data + dram_entry.size);
		}
	};

	class read_accessor {
	private:
		mpsc_queue *queue;
		size_t len;
		char *data;

		// XXX - Input iterator
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
		read_accessor(mpsc_queue *q)
		{
			queue = q;
			size_t offset;

			len = ringbuf_consume(queue->ring_buffer, &offset);
			data = queue->buf + offset;
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

		~read_accessor()
		{
			ringbuf_release(queue->ring_buffer, len);
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
			w = ringbuf_register(queue->ring_buffer, id);
		}

		~worker()
		{
			ringbuf_unregister(queue->ring_buffer, w);
			auto &manager = queue->get_id_manager();
			manager.release(id);
		}

		template <typename Function>
		bool
		produce(size_t size, Function &&f)
		{
			auto acc = accessor(queue, w, size);
			auto data = acc.get_range();
			f(data);
			return true;
		}
	};

private:
	inline pmem::detail::id_manager &
	get_id_manager()
	{
		static pmem::detail::id_manager manager;
		return manager;
	}

	ringbuf::ringbuf_t *ring_buffer;
	char *buf;
	pmem::obj::pool_base pop;
	size_t buff_size_;

public:
	mpsc_queue(pmem::obj::persistent_ptr<char[]> log, size_t buff_size,
		   size_t max_workers = 1)
	{
		size_t ring_buffer_size;

		auto addr = (uintptr_t)log.get();
		auto aligned_addr = ALIGN_UP(addr, CACHELINE_SIZE);

		buf = (char *)aligned_addr;
		pop = pmem::obj::pool_by_pptr(log);
		buff_size_ = buff_size - (aligned_addr - addr);

		ringbuf::ringbuf_get_sizes(max_workers, &ring_buffer_size,
					   NULL);
		ring_buffer = (ringbuf::ringbuf_t *)malloc(ring_buffer_size);
		ringbuf::ringbuf_setup(ring_buffer, max_workers, buff_size);
	}

	~mpsc_queue()
	{
		free(ring_buffer);
	}

	worker
	register_worker()
	{
		return worker(this);
	}

	template <typename Function>
	bool
	consume(Function &&f)
	{
		auto acc = read_accessor(this);
		f(acc);
		return true;
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
