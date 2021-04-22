#ifndef LIBPMEMOBJ_MPSC_QUEUE_HPP
#define LIBPMEMOBJ_MPSC_QUEUE_HPP

#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>

// XXX: Move id_manager to separate file
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>

#include <libpmemobj++/container/detail/ringbuf/ringbuf.h>
#include <cstddef>
#include <cstring>
#include <atomic>

namespace pmem
{

namespace obj
{

namespace experimental
{


class mpsc_queue
{
public:
	class accessor
	{
	private:
		ringbuf_worker_t *w;
		mpsc_queue *queue;

		size_t accessor_window;
		char* data;
	public:

		accessor(mpsc_queue *q, ringbuf_worker_t *worker, size_t len)
		{
			accessor_window = len;
			w = worker;
			queue = q;
			auto offset = ringbuf_acquire(queue->ring_buffer, w, len);
			data = queue->buf.get() + offset;
		};

		char* add(const char* d, size_t len)
		{
			assert(len <= accessor_window);

			char* current_data = data;
			queue->pop->memcpy_persist(data ,d, len);
			data += len;
			accessor_window -= len;
			return current_data;
		}

		~accessor()
		{
			ringbuf_produce(queue->ring_buffer, w);
		}
	};

	class read_accessor
	{
	private:
		mpsc_queue *queue;
	public:
		size_t len;
		char* data;

		read_accessor(mpsc_queue *q)
		{
			queue = q;
			size_t offset;

			len = ringbuf_consume(queue->ring_buffer, &offset);
			data = queue->buf.get() + offset;
		}

		~read_accessor()
		{
			ringbuf_release(queue->ring_buffer, len);
		}
	};

	class worker
	{
	private:
		mpsc_queue *queue;
		ringbuf_worker_t *w;
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

		accessor produce(size_t len)
		{
			return accessor(queue, w, len);
		}

	};

private:

	inline pmem::detail::id_manager &get_id_manager()
	{
		static pmem::detail::id_manager manager;
		return manager;
	}

	ringbuf_t *ring_buffer;
	static pmem::detail::id_manager id_manager;
	pmem::obj::persistent_ptr<char[]> buf;
	pmem::obj::pool_base *pop;
	size_t buff_size_;


public:


	mpsc_queue(pmem::obj::pool_base *my_pool, pmem::obj::persistent_ptr<char[]> *log, size_t buff_size,size_t max_workers=1)
	{
		size_t ring_buffer_size;
		buf = *log;
		pop = my_pool;
		buff_size_ = buff_size;

		ringbuf_get_sizes(max_workers, &ring_buffer_size, NULL);
		ring_buffer = (ringbuf_t*) malloc(ring_buffer_size);
		ringbuf_setup(ring_buffer, max_workers, buff_size);
	}

	~mpsc_queue()
	{
		free(ring_buffer);
	}

	worker register_worker()
	{
		return worker(this);
	}

	read_accessor consume()
	{
		return read_accessor(this);
	}

	void recover()
	{
		auto recovery_worker = register_worker();
		// reclame all data in buffer
		auto useless_acc = recovery_worker.produce(buff_size_);
		(void)useless_acc;
	}
};

}
}
}

#endif /* LIBPMEMOBJ_MPSC_QUEUE_HPP */
