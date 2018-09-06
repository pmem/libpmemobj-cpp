/*
 * Copyright 2015-2018, Intel Corporation
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

#include <iostream>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_array.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <objcpp_examples_common.hpp>
#include <string.h>

using namespace pmem;
using namespace pmem::obj;
using namespace std;

#define MAX_BUFFLEN 30
#define POOLSIZE 1024 * 1024 * 64

namespace
{
// array_op: available array operations
enum array_op {
	UNKNOWN_ARRAY_OP,
	ARRAY_PRINT,
	ARRAY_FREE,
	ARRAY_REALLOC,
	ARRAY_ALLOC,

	MAX_ARRAY_OP
};

const std::string LAYOUT = "";

// parse_array_op: parses the operation string and returns the matching array_op
array_op
parse_array_op(char *str)
{
	if (strcmp(str, "print") == 0)
		return ARRAY_PRINT;
	else if (strcmp(str, "free") == 0)
		return ARRAY_FREE;
	else if (strcmp(str, "realloc") == 0)
		return ARRAY_REALLOC;
	else if (strcmp(str, "alloc") == 0)
		return ARRAY_ALLOC;
	else
		return UNKNOWN_ARRAY_OP;
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

	persistent_ptr<array_list> head = NULL;

public:
	// add_array: allocate space on heap for new array and add it to head
	void
	add_array(pool_base &pop, const char *name, size_t size)
	{
		if (find_array(name) != NULL) {
			cout << "Array with name: " << name
			     << " already exists. ";
			cout << "If you prefer, you can reallocate this array "
			     << endl;
			printUsage(ARRAY_REALLOC, (char *)"./example-array");
		} else if (size < 1) {
			cout << "size must be a non-negative integer" << endl;
			printUsage(ARRAY_ALLOC, (char *)"./example-array");
		} else {
			transaction::exec_tx(pop, [&] {
				auto newArray = make_persistent<array_list>();
				strncpy(newArray->name, (const char *)name,
					MAX_BUFFLEN);
				newArray->name[MAX_BUFFLEN - 1] = '\0';
				newArray->size = size;
				newArray->array = make_persistent<int[]>(size);
				newArray->next = NULL;

				// assign values to newArray->array
				for (size_t i = 0; i < newArray->size; i++) {
					newArray->array[i] = i;
				}

				if (head == NULL) {
					head = newArray;
					return;
				}
				persistent_ptr<array_list> cur = head;
				while (cur->next) {
					cur = cur->next;
				}
				cur->next = newArray;
			});
		}
	}

	// delete_array: deletes array from the array_list and removes
	// previously allocated space on heap
	void
	delete_array(pool_base &pop, const char *name)
	{
		persistent_ptr<array_list> arr = find_array(name);

		if (arr == NULL) {
			cout << "No array found with name: " << name << endl;
			return;
		}

		persistent_ptr<array_list> cur = head;
		persistent_ptr<array_list> prev = head;

		while (cur) {
			if (list_equal(cur, arr)) {
				break;
			}
			prev = cur;
			cur = cur->next;
		}

		transaction::exec_tx(pop, [&] {
			if (list_equal(head, cur)) {
				head = cur->next;
			} else {
				prev->next = arr->next;
			}
			delete_persistent<int[]>(cur->array, cur->size);
			delete_persistent<array_list>(cur);
		});
	}

	// print_array: prints array_list contents to cout
	void
	print_array(const char *name)
	{
		persistent_ptr<array_list> arr = find_array(name);
		if (arr == NULL)
			cout << "No array found with name: " << name << endl;
		else {
			cout << arr->name << " = [";
			for (size_t i = 0; i < arr->size - 1; i++) {
				cout << arr->array[i] << ", ";
			}
			cout << arr->array[arr->size - 1] << "]" << endl;
		}
	}

	// resize: reallocate space on heap to change the size of the array.
	void
	resize(pool_base &pop, const char *name, size_t size)
	{
		persistent_ptr<array_list> arr = find_array(name);
		if (arr == NULL) {
			cout << "No array found with name: " << name << endl;
		} else if (size < 1) {
			cout << "size must be a non-negative integer" << endl;
			printUsage(ARRAY_REALLOC, (char *)"./example-array");
		} else {
			transaction::exec_tx(pop, [&] {
				persistent_ptr<int[]> newArray =
					make_persistent<int[]>(size);
				size_t copySize = arr->size;
				if (size < arr->size)
					copySize = size;
				for (size_t i = 0; i < copySize; i++) {
					newArray[i] = arr->array[i];
				}
				delete_persistent<int[]>(arr->array, arr->size);
				arr->size = size;
				arr->array = newArray;
			});
		}
	}

	// printUsage: prints usage for each type of array opperation
	void
	printUsage(array_op op, char *arg_zero)
	{
		switch (op) {
			case ARRAY_PRINT:
				std::cerr << "print array usage: " << arg_zero
					  << " <file_name> print <array_name>"
					  << std::endl;
				break;
			case ARRAY_FREE:
				std::cerr << "free array usage: " << arg_zero
					  << " <file_name> free <array_name>"
					  << std::endl;
				break;
			case ARRAY_REALLOC:
				std::cerr << "realloc array usage: " << arg_zero
					  << " <file_name> realloc "
					     "<array_name> <size>"
					  << std::endl;
				break;
			case ARRAY_ALLOC:
				std::cerr << "alloc array usage: " << arg_zero
					  << " <file_name> alloc <array_name> "
					     "<size>"
					  << std::endl;
				break;
			default:
				std::cerr << "usage: " << arg_zero
					  << " <file_name> "
					     "[print|alloc|free|realloc] "
					     "<array_name>"
					  << std::endl;
		};
		return;
	}

private:
	// list_equal: returns true of array_lists have the same name, size, and
	// array values
	bool
	list_equal(persistent_ptr<array_list> a, persistent_ptr<array_list> b)
	{
		if (strcmp(a->name, b->name) == 0)
			if (a->size == b->size)
				if (a->array == b->array)
					return true;
		return false;
	}

	// find_array: loops through head to find array with specified name
	persistent_ptr<array_list>
	find_array(const char *name)
	{
		char trimmedName[MAX_BUFFLEN];
		strncpy(trimmedName, (const char *)name, MAX_BUFFLEN);
		trimmedName[MAX_BUFFLEN - 1] = '\0';

		if (head == NULL)
			return head;
		persistent_ptr<array_list> cur = head;
		while (cur) {
			if (strcmp(cur->name, trimmedName) == 0) {
				return cur;
			}
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
	 *   ./example-array.cpp <file_name> print <array_name>
	 *   ./example-array.cpp <file_name> free <array_name>
	 *   ./example-array.cpp <file_name> realloc <array_name> <size>
	 *   ./example-array.cpp <file_name> alloc <array_name> <size>
	 *           // currently only enabled for arrays of int
	 */

	if (argc < 4) {
		std::cerr << "usage: " << argv[0]
			  << " <file_name> [print|alloc|free|realloc] "
			     "<array_name>"
			  << std::endl;
		return 1;
	}

	const char *file = argv[1];
	pool<examples::pmem_array> pop;

	if (file_exists(file) != 0) {
		pop = pool<examples::pmem_array>::create(file, LAYOUT, POOLSIZE,
							 CREATE_MODE_RW);
	} else {
		pop = pool<examples::pmem_array>::open(file, LAYOUT);
	}

	persistent_ptr<examples::pmem_array> arr = pop.get_root();

	array_op op = parse_array_op(argv[2]);

	switch (op) {
		case ARRAY_PRINT:
			if (argc == 4)
				arr->print_array(argv[3]);
			else
				arr->printUsage(op, argv[0]);
			break;
		case ARRAY_FREE:
			if (argc == 4)
				arr->delete_array(pop, argv[3]);
			else
				arr->printUsage(op, argv[0]);
			break;
		case ARRAY_REALLOC:
			if (argc == 5)
				arr->resize(pop, argv[3],
					    (size_t)atoi(argv[4]));
			else
				arr->printUsage(op, argv[0]);
			break;
		case ARRAY_ALLOC:
			if (argc == 5)
				arr->add_array(pop, argv[3],
					       (size_t)atoi(argv[4]));
			else
				arr->printUsage(op, argv[0]);
			break;
		default:
			cout << "Ruh roh! You passed an invalid operation!!"
			     << endl;
			arr->printUsage(op, argv[0]);
			break;
	}

	pop.close();
}
