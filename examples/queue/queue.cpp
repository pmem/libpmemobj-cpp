// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2015-2020, Intel Corporation */

/*
 * queue.cpp -- queue example implemented using pmemobj cpp bindings
 *
 * Please see pmem.io blog posts for more details.
 */

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj_cpp_examples_common.hpp>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

#define LAYOUT "queue"

namespace
{

/* available queue operations */
enum queue_op {
	UNKNOWN_QUEUE_OP,
	QUEUE_PUSH,
	QUEUE_POP,
	QUEUE_SHOW,

	MAX_QUEUE_OP,
};

/* queue operations strings */
const char *ops_str[MAX_QUEUE_OP] = {"", "push", "pop", "show"};

/*
 * parse_queue_op -- parses the operation string and returns matching queue_op
 */
queue_op
parse_queue_op(const char *str)
{
	for (int i = 0; i < MAX_QUEUE_OP; ++i)
		if (strcmp(str, ops_str[i]) == 0)
			return (queue_op)i;

	return UNKNOWN_QUEUE_OP;
}
}

using pmem::obj::delete_persistent;
using pmem::obj::make_persistent;
using pmem::obj::p;
using pmem::obj::persistent_ptr;
using pmem::obj::pool;
using pmem::obj::pool_base;
using pmem::obj::transaction;

namespace examples
{

/*
 * Persistent memory list-based queue
 *
 * A simple, not template based, implementation of queue using
 * libpmemobj C++ API. It demonstrates the basic features of persistent_ptr<>
 * and p<> classes.
 */
class pmem_queue {

	/* entry in the list */
	struct pmem_entry {
		persistent_ptr<pmem_entry> next;
		p<uint64_t> value;
	};

public:
	/*
	 * Inserts a new element at the end of the queue.
	 */
	void
	push(pool_base &pop, uint64_t value)
	{
		transaction::run(pop, [&] {
			auto n = make_persistent<pmem_entry>();

			n->value = value;
			n->next = nullptr;

			if (head == nullptr && tail == nullptr) {
				head = tail = n;
			} else {
				tail->next = n;
				tail = n;
			}
		});
	}

	/*
	 * Removes the first element in the queue.
	 */
	uint64_t
	pop(pool_base &pop)
	{
		uint64_t ret = 0;
		transaction::run(pop, [&] {
			if (head == nullptr)
				transaction::abort(EINVAL);

			ret = head->value;
			auto n = head->next;

			delete_persistent<pmem_entry>(head);
			head = n;

			if (head == nullptr)
				tail = nullptr;
		});

		return ret;
	}

	/*
	 * Prints the entire contents of the queue.
	 */
	void
	show(void) const
	{
		for (auto n = head; n != nullptr; n = n->next)
			std::cout << n->value << std::endl;
	}

private:
	persistent_ptr<pmem_entry> head;
	persistent_ptr<pmem_entry> tail;
};

} /* namespace examples */

int
main(int argc, char *argv[])
{
	if (argc < 3) {
		std::cerr << "usage: " << argv[0]
			  << " file-name [push [value]|pop|show]" << std::endl;
		return 1;
	}

	const char *path = argv[1];

	queue_op op = parse_queue_op(argv[2]);

	pool<examples::pmem_queue> pop;
	persistent_ptr<examples::pmem_queue> q;

	try {
		if (file_exists(path) != 0) {
			pop = pool<examples::pmem_queue>::create(
				path, LAYOUT, PMEMOBJ_MIN_POOL, CREATE_MODE_RW);
		} else {
			pop = pool<examples::pmem_queue>::open(path, LAYOUT);
		}
		q = pop.root();
	} catch (const pmem::pool_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const pmem::transaction_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}

	switch (op) {
		case QUEUE_PUSH:
			try {
				q->push(pop, std::stoull(argv[3]));
			} catch (const std::runtime_error &e) {
				std::cerr << "Exception: " << e.what()
					  << std::endl;
				return 1;
			}
			break;
		case QUEUE_POP:
			try {
				std::cout << q->pop(pop) << std::endl;
			} catch (const std::runtime_error &e) {
				std::cerr << "Exception: " << e.what()
					  << std::endl;
				return 1;
			} catch (const std::logic_error &e) {
				std::cerr << "Exception: " << e.what()
					  << std::endl;
				return 1;
			}
			break;
		case QUEUE_SHOW:
			q->show();
			break;
		default:
			std::cerr << "Invalid queue operation" << std::endl;
			return 1;
	}

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
