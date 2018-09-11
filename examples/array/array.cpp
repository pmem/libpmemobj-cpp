/*
 * Copyright 2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * array.cpp -- array example implemented using libpmemobj C++ bindings
 */

#include "objcpp_examples_common.hpp"
#include <iostream>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <string.h>

using namespace pmem;
using namespace pmem::obj;

#define POOLSIZE 1024 * 1024 * 64

namespace
{
// array_op: available array operations
enum class array_op {
	UNKNOWN,
	PRINT,
	FREE,
	REALLOC,
	ALLOC,

	MAX_ARRAY_OP
};

const int MAX_BUFFLEN = 30;
const std::string LAYOUT = "";

// parse_array_op: parses the operation string and returns the matching array_op
array_op
parse_array_op(char *str)
{
	if (strcmp(str, "print") == 0)
		return array_op::PRINT;
	else if (strcmp(str, "free") == 0)
		return array_op::FREE;
	else if (strcmp(str, "realloc") == 0)
		return array_op::REALLOC;
	else if (strcmp(str, "alloc") == 0)
		return array_op::ALLOC;
	else
		return array_op::UNKNOWN;
}
}

namespace examples
{

class pmem_array {
	// array_list: struct to hold name, size, array and pointer to next
	struct array_list {
		char name[MAX_BUFFLEN];
		p<size_t> size;
		persistent_ptr<int[]> array;
		persistent_ptr<array_list> next;
	};

	persistent_ptr<array_list> head = nullptr;

public:
	// add_array: allocate space on heap for new array and add it to head
	void
	add_array(pool_base &pop, const char *name, int size)
	{
		if (find_array(name) != nullptr) {
			std::cout << "Array with name: " << name
				  << " already exists. ";
			std::cout << "If you prefer, you can reallocate this "
				     "array "
				  << std::endl;
			print_usage(array_op::REALLOC, "./example-array");
		} else if (size < 1) {
			std::cout << "size must be a non-negative integer"
				  << std::endl;
			print_usage(array_op::ALLOC, "./example-array");
		} else {
			transaction::exec_tx(pop, [&] {
				auto newArray = make_persistent<array_list>();
				// strncpy(newArray->name, name,MAX_BUFFLEN);
				// newArray->name[MAX_BUFFLEN - 1] = '\0';
				strcpy(newArray->name, name);
				newArray->size = (size_t)size;
				newArray->array = make_persistent<int[]>(size);
				newArray->next = nullptr;

				// assign values to newArray->array
				for (size_t i = 0; i < newArray->size; i++)
					newArray->array[i] = i;

				newArray->next = head;
				head = newArray;
			});
		}
	}

	// delete_array: deletes array from the array_list and removes
	// previously allocated space on heap
	void
	delete_array(pool_base &pop, const char *name)
	{
		// prevArr will equal head if array wanted is either first OR
		// second element
		persistent_ptr<array_list> prevArr = find_array(name, true);

		// if array_list length = 0 OR array not found in list
		if (prevArr == nullptr) {
			std::cout << "No array found with name: " << name
				  << std::endl;
			return;
		}

		persistent_ptr<array_list> curArr;
		if (strcmp(prevArr->name, name) == 0) {
			// cur = prev= head, maybe only one element in list
			curArr = head;
		} else
			curArr = prevArr->next;

		transaction::exec_tx(pop, [&] {
			if (list_equal(head, curArr))
				head = curArr->next;
			else
				prevArr->next = curArr->next;
			delete_persistent<int[]>(curArr->array, curArr->size);
			delete_persistent<array_list>(curArr);
		});
	}

	// print_array: prints array_list contents to cout
	void
	print_array(const char *name)
	{
		persistent_ptr<array_list> arr = find_array(name);
		if (arr == nullptr)
			std::cout << "No array found with name: " << name
				  << std::endl;
		else {
			std::cout << arr->name << " = [";
			for (size_t i = 0; i < arr->size - 1; i++)
				std::cout << arr->array[i] << ", ";
			std::cout << arr->array[arr->size - 1] << "]"
				  << std::endl;
		}
	}

	// resize: reallocate space on heap to change the size of the array
	void
	resize(pool_base &pop, const char *name, int size)
	{
		persistent_ptr<array_list> arr = find_array(name);
		if (arr == nullptr)
			std::cout << "No array found with name: " << name
				  << std::endl;
		else if (size < 1) {
			std::cout << "size must be a non-negative integer"
				  << std::endl;
			print_usage(array_op::REALLOC, "./example-array");
		} else {
			transaction::exec_tx(pop, [&] {
				persistent_ptr<int[]> newArray =
					make_persistent<int[]>(size);
				size_t copySize = arr->size;
				if ((size_t)size < arr->size)
					copySize = (size_t)size;
				for (size_t i = 0; i < copySize; i++)
					newArray[i] = arr->array[i];
				delete_persistent<int[]>(arr->array, arr->size);
				arr->size = (size_t)size;
				arr->array = newArray;
			});
		}
	}

	// print_usage: prints usage for each type of array operation
	void
	print_usage(array_op op, const char *arg_zero)
	{
		switch (op) {
			case array_op::PRINT:
				std::cerr << "print array usage: " << arg_zero
					  << " <file_name> print "
					     "<array_name>"
					  << std::endl;
				break;
			case array_op::FREE:
				std::cerr << "free array usage: " << arg_zero
					  << " <file_name> free "
					     "<array_name>"
					  << std::endl;
				break;
			case array_op::REALLOC:
				std::cerr << "realloc array usage: " << arg_zero
					  << " <file_name> realloc "
					     "<array_name> <size>"
					  << std::endl;
				break;
			case array_op::ALLOC:
				std::cerr << "alloc array usage: " << arg_zero
					  << " <file_name> alloc "
					     "<array_name> "
					     "<size>"
					  << std::endl;
				break;
			default:
				std::cerr << "usage: " << arg_zero
					  << " <file_name> "
					     "<print|alloc|free|realloc> "
					     "<array_name>"
					  << std::endl;
		};
		return;
	}

private:
	// list_equal: returns true if array_lists have the same name, size, and
	// array values
	bool
	list_equal(persistent_ptr<array_list> a, persistent_ptr<array_list> b)
	{
		if (strcmp(a->name, b->name) == 0 && (a->size == b->size) &&
		    (a->array == b->array))
			return true;
		else
			return false;
	}

	// find_array: loops through head to find array with specified name
	persistent_ptr<array_list>
	find_array(const char *name, bool findPrev = false)
	{
		if (head == nullptr)
			return head;
		persistent_ptr<array_list> cur = head;
		persistent_ptr<array_list> prev = head;

		while (cur) {
			if (strcmp(cur->name, name) == 0) {
				if (findPrev)
					return prev;
				else
					return cur;
			}
			prev = cur;
			cur = cur->next;
		}
		return (nullptr);
	}
};
}

int
main(int argc, char *argv[])
{
	/* Inputs should be one of:
	 *   ./example-array <file_name> print <array_name>
	 *   ./example-array <file_name> free <array_name>
	 *   ./example-array <file_name> realloc <array_name> <size>
	 *   ./example-array <file_name> alloc <array_name> <size>
	 *           // currently only enabled for arrays of int
	 */

	if (argc < 4) {
		std::cerr << "usage: " << argv[0]
			  << " <file_name> <print|alloc|free|realloc> "
			     "<array_name>"
			  << std::endl;
		return 1;
	}

	// check length of array name to ensure doesn't exceed buffer.
	const char *name = argv[3];
	if (strlen(name) > MAX_BUFFLEN) {
		std::cout << "Name exceeds buffer length of 30 characters. "
			     "Please "
			     "shorten and try again."
			  << std::endl;
		return 1;
	}

	const char *file = argv[1];
	pool<examples::pmem_array> pop;

	if (file_exists(file) != 0)
		pop = pool<examples::pmem_array>::create(file, LAYOUT, POOLSIZE,
							 CREATE_MODE_RW);
	else
		pop = pool<examples::pmem_array>::open(file, LAYOUT);

	persistent_ptr<examples::pmem_array> arr = pop.get_root();

	array_op op = parse_array_op(argv[2]);

	switch (op) {
		case array_op::PRINT:
			if (argc == 4)
				arr->print_array(name);
			else
				arr->print_usage(op, argv[0]);
			break;
		case array_op::FREE:
			if (argc == 4)
				arr->delete_array(pop, name);
			else
				arr->print_usage(op, argv[0]);
			break;
		case array_op::REALLOC:
			if (argc == 5)
				arr->resize(pop, name, atoi(argv[4]));
			else
				arr->print_usage(op, argv[0]);
			break;
		case array_op::ALLOC:
			if (argc == 5)
				arr->add_array(pop, name, atoi(argv[4]));
			else
				arr->print_usage(op, argv[0]);
			break;
		default:
			std::cout
				<< "Ruh roh! You passed an invalid operation!!"
				<< std::endl;
			arr->print_usage(op, argv[0]);
			break;
	}

	pop.close();
}
