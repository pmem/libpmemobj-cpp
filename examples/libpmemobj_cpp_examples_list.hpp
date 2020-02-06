// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2018, Intel Corporation */

/*
 * list.hpp -- Implementation of list
 */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_LIST_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_LIST_HPP

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>

namespace examples
{

template <typename T>
class list {
	class list_entry {
	public:
		list_entry() = delete;
		list_entry(pmem::obj::persistent_ptr<list_entry> previous,
			   pmem::obj::persistent_ptr<T> value)
		{
			val = value;
			next = nullptr;
			prev = previous;
		}

		pmem::obj::persistent_ptr<list_entry> prev;
		pmem::obj::persistent_ptr<list_entry> next;
		pmem::obj::persistent_ptr<T> val;
	};

public:
	list()
	{
		head = nullptr;
		tail = head;
		len = 0;
	}

	/**
	 * Push back the new element.
	 */
	void
	push_back(pmem::obj::persistent_ptr<T> val)
	{
		auto tmp = pmem::obj::make_persistent<list_entry>(tail, val);
		if (head == nullptr)
			head = tmp;
		else
			tail->next = tmp;
		tail = tmp;
		++len;
	}

	/**
	 * Pop the last element out from the list and return
	 * the pointer to it
	 */
	pmem::obj::persistent_ptr<T>
	pop_back()
	{
		assert(head != nullptr);
		auto tmp = tail;
		tail = tmp->prev;
		if (tail == nullptr)
			head = tail;
		else
			tail->next = nullptr;
		return tmp->val;
	}

	/**
	 * Return the pointer to the next element
	 */
	pmem::obj::persistent_ptr<list_entry>
	erase(unsigned id)
	{
		return remove_elm(get_elm(id));
	}

	/* clear - clear the whole list */
	void
	clear()
	{
		while (head != nullptr) {
			auto e = head;
			head = remove_elm(e);
		}
	}

	/**
	 * Get element with given id in list
	 */
	pmem::obj::persistent_ptr<T>
	get(unsigned id)
	{
		auto elm = get_elm(id);
		if (elm == nullptr)
			return nullptr;
		return elm->val;
	}

	/**
	 * Return number of elements in list
	 */
	unsigned
	size() const
	{
		return len;
	}

private:
	pmem::obj::persistent_ptr<list_entry>
	get_elm(unsigned id)
	{
		if (id >= len)
			return nullptr;
		auto tmp = head;
		for (unsigned i = 0; i < id; i++)
			tmp = tmp->next;
		return tmp;
	}

	pmem::obj::persistent_ptr<list_entry>
	remove_elm(pmem::obj::persistent_ptr<list_entry> elm)
	{
		assert(elm != nullptr);
		auto tmp = elm->next;
		pmem::obj::delete_persistent<T>(elm->val);

		/* removing item is head */
		if (elm == head)
			head = elm->next;
		else
			elm->prev->next = elm->next;

		/* removing item is tail */
		if (elm == tail)
			tail = elm->prev;
		else
			elm->next->prev = elm->prev;

		--len;
		pmem::obj::delete_persistent<list_entry>(elm);
		return tmp;
	}

	pmem::obj::p<unsigned> len;
	pmem::obj::persistent_ptr<list_entry> head;
	pmem::obj::persistent_ptr<list_entry> tail;
};
};

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_LIST_HPP */
