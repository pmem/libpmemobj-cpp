// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/**
 * @file
 * Implementation of persistent multi producer single consumer queue.
 */

#ifndef LIBPMEMOBJ_MPSC_QUEUE_HPP
#define LIBPMEMOBJ_MPSC_QUEUE_HPP

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/enumerable_thread_specific.hpp>
#include <libpmemobj++/detail/ringbuf.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
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

/**
 * Persistent memory aware implementation of multi producer single consumer
 * queue.
 *
 * In case of crash or shutdown, reading and writing may be continued
 * by a new process, from the last position, without losing any
 * already produced data.
 *
 * @note try_consume_batch() MUST be called after creation of mpsc_queue object
 * if pmem_log_type object was already used by any instance of mpsc_queue - e.g.
 * in previous run of the application. If try_consume_batch() is not called,
 * produce may fail, even if the queue is empty.
 *
 * Example:
 * @snippet mpsc_queue/mpsc_queue.cpp mpsc_queue_single_threaded_example
 * @ingroup experimental_containers
 */
class mpsc_queue {
public:
	class worker;
	class pmem_log_type;
	class batch_type;

	mpsc_queue(pmem_log_type &pmem, size_t max_workers = 1);

	worker register_worker();

	template <typename Function>
	bool try_consume_batch(Function &&f);

private:
	struct first_block {
		static constexpr size_t CAPACITY =
			pmem::detail::CACHELINE_SIZE - sizeof(size_t);
		static constexpr size_t DIRTY_FLAG =
			(1ULL << (sizeof(size_t) * 8 - 1));

		pmem::obj::p<size_t> size;
		char data[CAPACITY];
	};

	struct iterator {
		iterator(char *data, char *end);

		iterator &operator++();

		bool operator==(const iterator &rhs) const;
		bool operator!=(const iterator &rhs) const;

		pmem::obj::string_view operator*() const;

	private:
		first_block *seek_next(first_block *);

		char *data;
		char *end;
	};

	void clear_cachelines(first_block *block, size_t size);
	void restore_offsets();

	size_t consume_cachelines(size_t *offset);
	void release_cachelines(size_t len);

	inline pmem::detail::id_manager &get_id_manager();

	/* ringbuf_t handle. Important: mpsc_queue operates on cachelines hence
	 * ringbuf_produce/release functions are called with number of
	 * cachelines, not bytes. */
	std::unique_ptr<ringbuf::ringbuf_t> ring_buffer;
	char *buf;
	pmem::obj::pool_base pop;
	size_t buf_size;
	pmem_log_type *pmem;

	/* Stores offset and length of next message to be consumed. Only
	 * valid if ring_buffer->consume_in_progress. */
	size_t consume_offset = 0;
	size_t consume_len = 0;

public:
	/**
	 * Type representing the range of the mpsc_queue elements. May be used
	 * in the range-based loops over accessed elements.
	 */
	class batch_type {
	public:
		batch_type(iterator begin, iterator end);

		iterator begin() const;
		iterator end() const;

	private:
		iterator begin_;
		iterator end_;
	};

	/**
	 * mpsc_queue producer worker class. To write data concurrently into the
	 * mpsc_queue in the multi-threaded application, each producer thread
	 * have to use its own worker object. Workers might be added
	 * concurrently to the mpsc_queue.
	 *
	 * @note All workers have to be destroyed before destruction of
	 * the mpsc_queue
	 *
	 * @see mpsc_queue:try_produce_batch()
	 */
	class worker {
	public:
		worker(mpsc_queue *q);
		~worker();

		worker(const worker &) = delete;
		worker &operator=(const worker &) = delete;

		worker(worker &&other);
		worker &operator=(worker &&other);

		template <typename Function = void (*)(pmem::obj::string_view)>
		bool try_produce(
			pmem::obj::string_view data,
			Function &&on_produce =
				[](pmem::obj::string_view target) {});

	private:
		mpsc_queue *queue;
		ringbuf::ringbuf_worker_t *w;
		size_t id;

		ptrdiff_t acquire_cachelines(size_t len);
		void produce_cachelines();
		void store_to_log(pmem::obj::string_view data, char *log_data);

		friend class mpsc_queue;
	};

	/**
	 * Type representing persistent data, which may be managed by
	 * mpsc_queue.
	 *
	 * Object of this type has to be managed by pmem::obj::pool, to be
	 * usable in mpsc_queue.
	 * Once created, pmem_log_type object cannot be resized.
	 *
	 * @param size size of the log.
	 */
	class pmem_log_type {
	public:
		pmem_log_type(size_t size);

		pmem::obj::string_view data();

	private:
		pmem::obj::vector<char> data_;
		pmem::obj::p<size_t> written;

		friend class mpsc_queue;
	};
};

/**
 * mpsc_queue constructor.
 *
 * @param[in] pmem reference to already allocated pmem_log_type object
 * @param[in] max_workers maximum number of workers which may be added to
 * mpsc_queue at the same time.
 */
mpsc_queue::mpsc_queue(pmem_log_type &pmem, size_t max_workers)
{
	pop = pmem::obj::pool_by_vptr(&pmem);

	auto buf_data = pmem.data();

	buf = const_cast<char *>(buf_data.data());
	buf_size = buf_data.size();

	assert(buf_size % pmem::detail::CACHELINE_SIZE == 0);

	ring_buffer =
		std::unique_ptr<ringbuf::ringbuf_t>(new ringbuf::ringbuf_t(
			max_workers, buf_size / pmem::detail::CACHELINE_SIZE));

	this->pmem = &pmem;

	restore_offsets();
}

ptrdiff_t
mpsc_queue::worker::acquire_cachelines(size_t len)
{
	assert(len % pmem::detail::CACHELINE_SIZE == 0);
	auto ret = ringbuf_acquire(queue->ring_buffer.get(), w,
				   len / pmem::detail::CACHELINE_SIZE);

	if (ret < 0)
		return ret;

	return ret * static_cast<ptrdiff_t>(pmem::detail::CACHELINE_SIZE);
}

void
mpsc_queue::worker::produce_cachelines()
{
	ringbuf_produce(queue->ring_buffer.get(), w);
}

size_t
mpsc_queue::consume_cachelines(size_t *offset)
{
	auto ret = ringbuf_consume(ring_buffer.get(), offset);
	if (ret) {
		*offset *= pmem::detail::CACHELINE_SIZE;
		return ret * pmem::detail::CACHELINE_SIZE;
	}

	return 0;
}

void
mpsc_queue::release_cachelines(size_t len)
{
	assert(len % pmem::detail::CACHELINE_SIZE == 0);
	ringbuf_release(ring_buffer.get(), len / pmem::detail::CACHELINE_SIZE);
}

void
mpsc_queue::restore_offsets()
{
	/* Invariant */
	assert(pmem->written < buf_size);

	/* XXX: implement restore_offset function in ringbuf */

	auto w = register_worker();

	if (!pmem->written) {
		/* If pmem->written == 0 it means that consumer should start
		 * reading from the beginning. There might be elements produced
		 * anywhere in the log. Since we want to prohibit any producers
		 * from overwriting the original content - mark the entire log
		 * as produced. */

		auto acq = w.acquire_cachelines(buf_size -
						pmem::detail::CACHELINE_SIZE);
		assert(acq == 0);
		(void)acq;

		w.produce_cachelines();

		return;
	}

	/* If pmem->written != 0 there still might be element in the log.
	 * Moreover, to guarantee proper order of elements on recovery, we must
	 * restore consumer offset. (If we would start consuming from the
	 * beginning of the log, we could consume newer elements first.) Offsets
	 * are restored by following operations:
	 *
	 * produce(pmem->written);
	 * consume();
	 * produce(size - pmem->written);
	 * produce(pmem->written - CACHELINE_SIZE);
	 *
	 * This results in producer offset equal to pmem->written -
	 * CACHELINE_SIZE and consumer offset equal to pmem->written.
	 */

	auto acq = w.acquire_cachelines(pmem->written);
	assert(acq == 0);
	w.produce_cachelines();

	/* Restore consumer offset */
	size_t offset;
	auto len = consume_cachelines(&offset);
	assert(len == pmem->written);
	release_cachelines(len);

	assert(offset == 0);
	assert(len == pmem->written);

	acq = w.acquire_cachelines(buf_size - pmem->written);
	assert(acq >= 0);
	assert(static_cast<size_t>(acq) == pmem->written);
	w.produce_cachelines();

	acq = w.acquire_cachelines(pmem->written -
				   pmem::detail::CACHELINE_SIZE);
	assert(acq == 0);
	(void)acq;
	w.produce_cachelines();
}

/**
 * Constructs pmem_log_type object
 *
 * @param size size of the log in bytes
 */
mpsc_queue::pmem_log_type::pmem_log_type(size_t size)
    : data_(size, 0), written(0)
{
}

/**
 * Returns  pmem::obj::string_view which allows to read-only access to the
 * underlying buffer.
 *
 * @return pmem::obj::string_view of the log data.
 */
inline pmem::obj::string_view
mpsc_queue::pmem_log_type::data()
{
	auto addr = reinterpret_cast<uintptr_t>(&data_[0]);
	auto aligned_addr =
		pmem::detail::align_up(addr, pmem::detail::CACHELINE_SIZE);

	auto size = data_.size() - (aligned_addr - addr);
	auto aligned_size =
		pmem::detail::align_down(size, pmem::detail::CACHELINE_SIZE);

	return pmem::obj::string_view(
		reinterpret_cast<const char *>(aligned_addr), aligned_size);
}

inline pmem::detail::id_manager &
mpsc_queue::get_id_manager()
{
	static pmem::detail::id_manager manager;
	return manager;
}

/**
 * Registers the producer worker. Number of workers have to be less or equal
 * to max_workers specified in the mpsc_queue constructor.
 *
 * @return producer worker object.
 *
 * @snippet mpsc_queue/mpsc_queue.cpp register_worker
 */
inline mpsc_queue::worker
mpsc_queue::register_worker()
{
	return worker(this);
}

/**
 * Evaluates callback function f() for the data, which is ready to be
 * consumed. try_consume_batch() accesses data and evaluates callback inside a
 * transaction. If an exception is thrown within callback, it gets
 * propagated to the caller and causes a transaction abort. In such case, next
 * try_consume_batch() call would consume the same data.
 *
 * @return true if consumed any data, false otherwise.
 *
 * @throws transaction_scope_error
 *
 * @note try_consume_batch() MUST be called after creation of mpsc_queue object
 * if pmem_log_type object was already used by any instance of mpsc_queue.
 * Otherwise produce might fail, even if the queue is empty.
 *
 * @see mpsc_queue::worker::try_produce()
 *
 * @snippet mpsc_queue/mpsc_queue.cpp try_consume_batch
 */
template <typename Function>
inline bool
mpsc_queue::try_consume_batch(Function &&f)
{
	if (pmemobj_tx_stage() != TX_STAGE_NONE)
		throw pmem::transaction_scope_error(
			"Function called inside a transaction scope.");

	bool consumed = false;

	/* Need to call try_consume twice, as some data may be at the end
	 * of buffer, and some may be at the beginning. Ringbuffer does not
	 * merge those two parts into one try_consume. If all data was
	 * consumed during first try_consume, second will do nothing. */
	for (int i = 0; i < 2; i++) {
		/* If there is no consume in progress, it's safe to call
		 * ringbuf_consume. */
		if (!ring_buffer->consume_in_progress) {
			size_t offset;
			auto len = consume_cachelines(&offset);
			if (!len)
				return consumed;

			consume_offset = offset;
			consume_len = len;
		} else {
			assert(consume_len != 0);
		}

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		ANNOTATE_HAPPENS_AFTER(ring_buffer.get());
#endif

		auto data = buf + consume_offset;
		auto begin = iterator(data, data + consume_len);
		auto end = iterator(data + consume_len, data + consume_len);

		pmem::obj::flat_transaction::run(pop, [&] {
			if (begin != end) {
				consumed = true;
				f(batch_type(begin, end));
			}

			auto b = reinterpret_cast<first_block *>(data);
			clear_cachelines(b, consume_len);

			if (consume_offset + consume_len < buf_size)
				pmem->written = consume_offset + consume_len;
			else if (consume_offset + consume_len == buf_size)
				pmem->written = 0;
			else
				assert(false);
		});

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
		ANNOTATE_HAPPENS_BEFORE(ring_buffer.get());
#endif

		release_cachelines(consume_len);

		assert(!ring_buffer->consume_in_progress);

		/* XXX: it would be better to call f once - hide
		 * wraparound behind iterators */
		/* XXX: add param to ringbuf_consume and do not
		 * call store_explicit in consume */
	}

	return consumed;
}

/**
 * Default constructor of worker object.
 * It's registered as a worker for passed mpsc_queue @param q .
 */
inline mpsc_queue::worker::worker(mpsc_queue *q)
{
	queue = q;
	auto &manager = queue->get_id_manager();

#if LIBPMEMOBJ_CPP_VG_DRD_ENABLED
	ANNOTATE_BENIGN_RACE_SIZED(
		&manager, sizeof(std::mutex),
		"https://bugs.kde.org/show_bug.cgi?id=416286");
#endif

	id = manager.get();

	assert(id < q->ring_buffer->nworkers);

	w = ringbuf_register(queue->ring_buffer.get(), id);
}

/**
 * Move constructor, from @param other worker.
 */
inline mpsc_queue::worker::worker(mpsc_queue::worker &&other)
{
	*this = std::move(other);
}

/**
 * Move assignment operator, from @param other worker.
 */
inline mpsc_queue::worker &
mpsc_queue::worker::operator=(worker &&other)
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

/**
 * Default destructor of worker object.
 * It unregisters the worker (in ringbuf and in queue's manager).
 */
inline mpsc_queue::worker::~worker()
{
	if (w) {
		ringbuf_unregister(queue->ring_buffer.get(), w);
		auto &manager = queue->get_id_manager();
		manager.release(id);
	}
}

/**
 * Copies data from pmem::obj::string_view into the mpsc_queue.
 *
 * @param[in] data Data to be copied into mpsc_queue
 * @param[in] on_produce Function evaluated on the data in queue, before
 * the data is visible for the consumer. By default do nothing.
 *
 * @return true if f were evaluated, all data copied by it saved in the
 *  mpsc_queue, and are visible for the consumer.
 *
 * @snippet mpsc_queue/mpsc_queue.cpp try_produce_string_view
 */
template <typename Function>
bool
mpsc_queue::worker::try_produce(pmem::obj::string_view data,
				Function &&on_produce)
{
	auto req_size =
		pmem::detail::align_up(data.size() + sizeof(first_block::size),
				       pmem::detail::CACHELINE_SIZE);
	auto offset = acquire_cachelines(req_size);

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	ANNOTATE_HAPPENS_AFTER(queue->ring_buffer.get());
#endif

	if (offset == -1)
		return false;

	store_to_log(data, queue->buf + offset);

#if LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED
	ANNOTATE_HAPPENS_BEFORE(queue->ring_buffer.get());
#endif

	on_produce(pmem::obj::string_view(
		queue->buf + offset + sizeof(first_block::size), data.size()));

	produce_cachelines();

	return true;
}

inline void
mpsc_queue::worker::store_to_log(pmem::obj::string_view data, char *log_data)
{
	assert(reinterpret_cast<uintptr_t>(log_data) %
		       pmem::detail::CACHELINE_SIZE ==
	       0);

/* Invariant: producer can only produce data to cachelines which have
 * first 8 bytes zeroed.
 */
#ifndef NDEBUG
	auto b = reinterpret_cast<first_block *>(log_data);
	auto s = pmem::detail::align_up(data.size() + sizeof(first_block::size),
					pmem::detail::CACHELINE_SIZE);
	auto e = b + s / pmem::detail::CACHELINE_SIZE;
	while (b < e) {
		assert(b->size == 0);
		b++;
	}
#endif

	assert(reinterpret_cast<first_block *>(log_data)->size == 0);

	first_block fblock;
	fblock.size = data.size() | size_t(first_block::DIRTY_FLAG);

	/*
	 * First step is to copy up to 56B of data and store
	 * data.size() with DIRTY flag set. After that, we store
	 * rest of the data in two steps:
	 *	1. Remainder of the data is aligned down to
	 *	cacheline and copied.
	 * Now, we are left with between 0 to 63 bytes. If
	 * nonzero:
	 *	2. Create a stack allocated cacheline-sized
	 *	buffer, fill in the remainder of the data, and
	 *	copy the entire cacheline. After all data is
	 *	stored, we clear the dirty flag from size.
	 *
	 * This is done so that we avoid a cache-miss on
	 * misaligned writes.
	 */

	size_t ncopy = (std::min)(data.size(), size_t(first_block::CAPACITY));
	std::copy_n(data.data(), ncopy, fblock.data);

	pmemobj_memcpy(queue->pop.handle(), log_data,
		       reinterpret_cast<char *>(&fblock),
		       pmem::detail::CACHELINE_SIZE, PMEMOBJ_F_MEM_NONTEMPORAL);

	size_t remaining_size = ncopy > data.size() ? 0 : data.size() - ncopy;

	const char *srcof = data.data() + ncopy;
	size_t rcopy = pmem::detail::align_down(remaining_size,
						pmem::detail::CACHELINE_SIZE);
	size_t lcopy = remaining_size - rcopy;

	char last_cacheline[pmem::detail::CACHELINE_SIZE];
	if (lcopy != 0)
		std::copy_n(srcof + rcopy, lcopy, last_cacheline);

	if (rcopy != 0) {
		char *dest = log_data + pmem::detail::CACHELINE_SIZE;

		pmemobj_memcpy(queue->pop.handle(), dest, srcof, rcopy,
			       PMEMOBJ_F_MEM_NODRAIN |
				       PMEMOBJ_F_MEM_NONTEMPORAL);
	}

	if (lcopy != 0) {
		void *dest = log_data + pmem::detail::CACHELINE_SIZE + rcopy;

		pmemobj_memcpy(queue->pop.handle(), dest, last_cacheline,
			       pmem::detail::CACHELINE_SIZE,
			       PMEMOBJ_F_MEM_NODRAIN |
				       PMEMOBJ_F_MEM_NONTEMPORAL);
	}

	pmemobj_drain(queue->pop.handle());

	fblock.size &= (~size_t(first_block::DIRTY_FLAG));

	pmemobj_memcpy(queue->pop.handle(), log_data,
		       reinterpret_cast<char *>(&fblock),
		       pmem::detail::CACHELINE_SIZE, PMEMOBJ_F_MEM_NONTEMPORAL);
}

/**
 * Constructs batch_type object.
 *
 * Represents the range (from @param begin_ to @param end_) of the
 * mpsc_queue elements.
 */
inline mpsc_queue::batch_type::batch_type(iterator begin_, iterator end_)
    : begin_(begin_), end_(end_)
{
}

/**
 * Returns an iterator to the beginning of the accessed range of the
 * mpsc_queue.
 *
 * @return Iterator to the first element.
 */
inline mpsc_queue::iterator
mpsc_queue::batch_type::begin() const
{
	return begin_;
}

/**
 * Returns an iterator to the end of the accessed range of the mpsc_queue.
 *
 * @return Iterator to the last element.
 */

inline mpsc_queue::iterator
mpsc_queue::batch_type::end() const
{
	return end_;
}

mpsc_queue::iterator::iterator(char *data, char *end) : data(data), end(end)
{
	auto b = reinterpret_cast<first_block *>(data);
	auto next = seek_next(b);
	assert(next >= b);
	this->data = reinterpret_cast<char *>(next);
}

void
mpsc_queue::clear_cachelines(first_block *block, size_t size)
{
	assert(size % pmem::detail::CACHELINE_SIZE == 0);
	assert(pmemobj_tx_stage() == TX_STAGE_WORK);

	auto end = block +
		static_cast<ptrdiff_t>(size / pmem::detail::CACHELINE_SIZE);

	while (block < end) {
		/* data in block might be uninitialized. */
		detail::conditional_add_to_tx(&block->size, 1,
					      POBJ_XADD_ASSUME_INITIALIZED);
		block->size = 0;
		block++;
	}

	assert(end <= reinterpret_cast<first_block *>(buf + buf_size));
}

mpsc_queue::iterator &
mpsc_queue::iterator::operator++()
{
	auto block = reinterpret_cast<first_block *>(data);
	assert(block->size != 0);

	auto element_size =
		pmem::detail::align_up(block->size + sizeof(block->size),
				       pmem::detail::CACHELINE_SIZE);

	block += element_size / pmem::detail::CACHELINE_SIZE;

	auto next = seek_next(block);
	assert(next >= block);
	block = next;

	data = reinterpret_cast<char *>(block);

	return *this;
}

bool
mpsc_queue::iterator::operator==(const mpsc_queue::iterator &rhs) const
{
	return data == rhs.data;
}

bool
mpsc_queue::iterator::operator!=(const mpsc_queue::iterator &rhs) const
{
	return data != rhs.data;
}

pmem::obj::string_view mpsc_queue::iterator::operator*() const
{
	auto b = reinterpret_cast<first_block *>(data);
	return pmem::obj::string_view(b->data, b->size);
}

mpsc_queue::first_block *
mpsc_queue::iterator::seek_next(mpsc_queue::first_block *b)
{
	auto e = reinterpret_cast<first_block *>(end);

	/* Advance to first, unconsumed element. Each cacheline can be in one of
	 * 3 states:
	 * 1. First 8 bytes (size) are equal to 0 - there is no data in this
	 * cacheline.
	 * 2. First 8 bytes (size) are non-zero and have dirty flag set - next
	 * size bytes are junk.
	 * 3. First 8 bytes (size) are non-zero and have dirty flag unset - next
	 * size bytes are ready to be consumed (they represent consistent data).
	 */
	while (b < e) {
		if (b->size == 0) {
			b++;
		} else if (b->size & size_t(first_block::DIRTY_FLAG)) {
			auto size =
				b->size & (~size_t(first_block::DIRTY_FLAG));
			auto aligned_size = pmem::detail::align_up(
				size + sizeof(b->size),
				pmem::detail::CACHELINE_SIZE);

			b += aligned_size / pmem::detail::CACHELINE_SIZE;
		} else {
			break;
		}
	}

	assert(b <= e);

	return b;
}

} /* namespace experimental */
} /* namespace obj */
} /* namespace pmem */

#endif /* LIBPMEMOBJ_MPSC_QUEUE_HPP */
