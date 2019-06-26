/*
 * Copyright 2018-2019, Intel Corporation
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

#include "libpmemobj_cpp_examples_common.hpp"
#include <cstring>
#include <iostream>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>

using namespace pmem;
using namespace pmem::obj;

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

const int POOLSIZE = 1024 * 1024 * 64;
const int MAX_BUFFLEN = 30;
const std::string LAYOUT = "";
std::string prog_name;

// parse_array_op: parses the operation string and returns the matching array_op
array_op
parse_array_op(const char *str)
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
			std::cout
				<< "If you prefer, you can reallocate this array "
				<< std::endl;
			print_usage(array_op::REALLOC, "./example-array");
		} else if (size < 1) {
			std::cout << "size must be a non-negative integer"
				  << std::endl;
			print_usage(array_op::ALLOC, "./example-array");
		} else {
			transaction::run(pop, [&] {
				auto new_array = make_persistent<array_list>();

				strcpy(new_array->name, name);

				new_array->size = (size_t)size;
				new_array->array = make_persistent<int[]>(size);
				new_array->next = nullptr;

				// assign values to newArray->array
				for (size_t i = 0; i < new_array->size; i++)
					new_array->array[i] = i;

				new_array->next = head;
				head = new_array;
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
		persistent_ptr<array_list> prev_arr = find_array(name, true);

		// if array_list length = 0 OR array not found in list
		if (prev_arr == nullptr) {
			std::cout << "No array found with name: " << name
				  << std::endl;
			return;
		}

		persistent_ptr<array_list> cur_arr;
		if (strcmp(prev_arr->name, name) == 0) {
			// cur = prev= head, either only one element in list or
			// array is first element
			cur_arr = head;
		} else {
			cur_arr = prev_arr->next;
		}

		transaction::run(pop, [&] {
			if (head == cur_arr)
				head = cur_arr->next;
			else
				prev_arr->next = cur_arr->next;

			delete_persistent<int[]>(cur_arr->array, cur_arr->size);
			delete_persistent<array_list>(cur_arr);
		});
	}

	// print_array: prints array_list contents to cout
	void
	print_array(const char *name)
	{
		persistent_ptr<array_list> arr = find_array(name);
		if (arr == nullptr) {
			std::cout << "No array found with name: " << name
				  << std::endl;
		} else {
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
		if (arr == nullptr) {
			std::cout << "No array found with name: " << name
				  << std::endl;
		} else if (size < 1) {
			std::cout << "size must be a non-negative integer"
				  << std::endl;
			print_usage(array_op::REALLOC, prog_name);
		} else {
			transaction::run(pop, [&] {
				persistent_ptr<int[]> new_array =
					make_persistent<int[]>(size);

				size_t copy_size = arr->size;

				if ((size_t)size < arr->size)
					copy_size = (size_t)size;

				for (size_t i = 0; i < copy_size; i++)
					new_array[i] = arr->array[i];

				delete_persistent<int[]>(arr->array, arr->size);

				arr->size = (size_t)size;
				arr->array = new_array;
			});
		}
	}

	// print_usage: prints usage for each type of array operation
	void
	print_usage(array_op op, std::string arg_zero)
	{
		switch (op) {
			case array_op::PRINT:
				std::cerr << "print array usage: " << arg_zero
					  << " <file_name> print <array_name>"
					  << std::endl;
				break;
			case array_op::FREE:
				std::cerr << "free array usage: " << arg_zero
					  << " <file_name> free <array_name>"
					  << std::endl;
				break;
			case array_op::REALLOC:
				std::cerr
					<< "realloc array usage: " << arg_zero
					<< " <file_name> realloc <array_name> <size>"
					<< std::endl;
				break;
			case array_op::ALLOC:
				std::cerr
					<< "alloc array usage: " << arg_zero
					<< " <file_name> alloc <array_name> <size>"
					<< std::endl;
				break;
			default:
				std::cerr
					<< "usage: " << arg_zero
					<< " <file_name> <print|alloc|free|realloc> <array_name>"
					<< std::endl;
		}
	}

private:
	// find_array: loops through head to find array with specified name
	persistent_ptr<array_list>
	find_array(const char *name, bool find_prev = false)
	{
		if (head == nullptr)
			return head;

		persistent_ptr<array_list> cur = head;
		persistent_ptr<array_list> prev = head;

		while (cur) {
			if (strcmp(cur->name, name) == 0) {
				if (find_prev)
					return prev;
				else
					return cur;
			}

			prev = cur;
			cur = cur->next;
		}

		return nullptr;
	}
};
}

int
main(int argc, char *argv[])
{
	/*
	 * Inputs should be one of:
	 *   ./example-array <file_name> print <array_name>
	 *   ./example-array <file_name> free <array_name>
	 *   ./example-array <file_name> realloc <array_name> <size>
	 *   ./example-array <file_name> alloc <array_name> <size>
	 *           // currently only enabled for arrays of int
	 */

	prog_name = argv[0];
	if (argc < 4) {
		std::cerr
			<< "usage: " << prog_name
			<< " <file_name> <print|alloc|free|realloc> <array_name>"
			<< std::endl;

		return 1;
	}

	// check length of array name to ensure doesn't exceed buffer.
	const char *name = argv[3];
	if (strlen(name) > MAX_BUFFLEN) {
		std::cout
			<< "Name exceeds buffer length of 30 characters. Please shorten and try again."
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

	persistent_ptr<examples::pmem_array> arr = pop.root();

	array_op op = parse_array_op(argv[2]);

	switch (op) {
		case array_op::PRINT:
			if (argc == 4)
				arr->print_array(name);
			else
				arr->print_usage(op, prog_name);
			break;
		case array_op::FREE:
			if (argc == 4)
				arr->delete_array(pop, name);
			else
				arr->print_usage(op, prog_name);
			break;
		case array_op::REALLOC:
			if (argc == 5)
				arr->resize(pop, name, atoi(argv[4]));
			else
				arr->print_usage(op, prog_name);
			break;
		case array_op::ALLOC:
			if (argc == 5)
				arr->add_array(pop, name, atoi(argv[4]));
			else
				arr->print_usage(op, prog_name);
			break;
		default:
			std::cout << "Ruh roh! You passed an invalid operation!"
				  << std::endl;

			arr->print_usage(op, prog_name);
			break;
	}

	pop.close();

	return 0;
}
