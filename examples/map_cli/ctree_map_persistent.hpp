// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2016-2021, Intel Corporation */

#ifndef LIBPMEMOBJ_CPP_EXAMPLES_CTREE_MAP_PERSISTENT_HPP
#define LIBPMEMOBJ_CPP_EXAMPLES_CTREE_MAP_PERSISTENT_HPP

#include <cstdint>
#include <cstdlib>
#include <functional>

#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/make_persistent_atomic.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>
#include <libpmemobj_cpp_examples_common.hpp>

#define BIT_IS_SET(n, i) (!!((n) & (1ULL << (i))))

namespace examples
{

/**
 * C++ implementation of a persistent ctree.
 *
 * Based on the volatile version. This version was implemented to show how much
 * effort is needed to convert a volatile structure into a persistent one using
 * C++ obj bindings. All API functions are atomic in respect to persistency.
 */
template <typename K, typename T>
class ctree_map_p {
public:
	/** Convenience typedef for the key type. */
	typedef K key_type;

	/** Convenience typedef for the value type. */
	typedef pmem::obj::persistent_ptr<T> value_type;

	/** Convenience typedef for the callback function. */
	typedef std::function<int(key_type, value_type, void *)> callback;

	/**
	 * Default constructor.
	 */
	ctree_map_p()
	{
		auto pop = pmem::obj::pool_by_vptr(this);

		pmem::obj::transaction::run(pop, [&] {
			this->root = pmem::obj::make_persistent<entry>();
		});
	}

	ctree_map_p(const ctree_map_p &other) = delete;

	ctree_map_p &operator=(const ctree_map_p &other) = delete;

	/**
	 * Insert or update the given value under the given key.
	 *
	 * The map takes ownership of the value.
	 *
	 * @param key The key to insert under.
	 * @param value The value to be inserted.
	 *
	 * @return 0 on success, negative values on error.
	 */
	int
	insert(key_type key, value_type value)
	{
		auto dest_entry = root;
		while (dest_entry->inode != nullptr) {
			auto n = dest_entry->inode;
			dest_entry = n->entries[BIT_IS_SET(key, n->diff)];
		}

		entry e(key, value);
		auto pop = pmem::obj::pool_by_vptr(this);
		pmem::obj::transaction::run(pop, [&] {
			if (dest_entry->key == 0 || dest_entry->key == key) {
				pmem::obj::delete_persistent<T>(
					dest_entry->value);
				*dest_entry = e;
			} else {
				insert_leaf(
					&e,
					find_crit_bit(dest_entry->key, key));
			}
		});

		return 0;
	}

	/**
	 * Allocating insert.
	 *
	 * Creates a new value_type instance and inserts it into the tree.
	 *
	 * @param key The key to insert under.
	 * @param args variadic template parameter for object construction
	 *	arguments.
	 *
	 * @return 0 on success, negative values on error.
	 */
	template <typename... Args>
	int
	insert_new(key_type key, const Args &... args)
	{
		auto pop = pmem::obj::pool_by_vptr(this);
		pmem::obj::transaction::run(pop, [&] {
			return insert(key,
				      pmem::obj::make_persistent<T>(args...));
		});

		return -1;
	}

	/**
	 * Remove a value from the tree.
	 *
	 * The tree no longer owns the value.
	 *
	 * @param key The key for which the value will be removed.
	 *
	 * @return The value if it is in the tree, nullptr otherwise.
	 */
	value_type
	remove(key_type key)
	{
		pmem::obj::persistent_ptr<entry> parent = nullptr;
		auto leaf = get_leaf(key, &parent);

		if (leaf == nullptr)
			return nullptr;

		auto ret = leaf->value;

		auto pop = pmem::obj::pool_by_vptr(this);
		pmem::obj::transaction::run(pop, [&] {
			if (parent == nullptr) {
				leaf->key = 0;
				leaf->value = nullptr;
			} else {
				auto n = parent->inode;
				*parent = *(
					n->entries[parent->inode->entries[0]
							   ->key == leaf->key]);

				/* cleanup entries and the unnecessary node */
				pmem::obj::delete_persistent<entry>(
					n->entries[0]);
				pmem::obj::delete_persistent<entry>(
					n->entries[1]);
				pmem::obj::delete_persistent<node>(n);
			}
		});

		return ret;
	}

	/**
	 * Remove entry from tree and deallocate it.
	 *
	 * @param key The key denoting the entry to be removed.
	 *
	 * @return 0 on success, negative values on error.
	 */
	int
	remove_free(key_type key)
	{
		auto pop = pmem::obj::pool_by_vptr(this);
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::delete_persistent<T>(remove(key));
		});
		return 0;
	}

	/**
	 * Clear the tree and deallocate all entries.
	 */
	int
	clear()
	{
		auto pop = pmem::obj::pool_by_vptr(this);
		pmem::obj::transaction::run(pop, [&] {
			if (this->root->inode) {
				this->root->inode->clear();
				pmem::obj::delete_persistent<node>(
					this->root->inode);
				this->root->inode = nullptr;
			}

			pmem::obj::delete_persistent<T>(this->root->value);
			this->root->value = nullptr;
			this->root->key = 0;
		});
		return 0;
	}

	/**
	 * Return the value from the tree for the given key.
	 *
	 * @param key The key for which the value will be returned.
	 *
	 * @return The value if it is in the tree, nullptr otherwise.
	 */
	value_type
	get(key_type key)
	{
		auto ret = get_leaf(key, nullptr);

		return ret ? ret->value : nullptr;
	}

	/**
	 * Check if an entry for the given key is in the tree.
	 *
	 * @param key The key to check.
	 *
	 * @return 0 on
	 */
	int
	lookup(key_type key) const
	{
		return get(key) != nullptr;
	}

	/**
	 * Call clb for each element in the tree.
	 *
	 * @param clb The callback to be called.
	 * @param args The arguments forwarded to the callback.
	 *
	 * @return 0 if tree empty, clb return value otherwise.
	 */
	int foreach (callback clb, void *args)
	{
		if (is_empty())
			return 0;

		return foreach_node(root, clb, args);
	}

	/**
	 * Check if tree is empty.
	 *
	 * @return 1 if empty, 0 otherwise.
	 */
	int
	is_empty() const
	{
		return root->value == nullptr && root->inode == nullptr;
	}

	/**
	 * Check tree consistency.
	 *
	 * @return 0 on success, negative values on error.
	 */
	int
	check() const
	{
		return 0;
	}

	/**
	 * Destructor.
	 */
	~ctree_map_p()
	{
		clear();
	}

private:
	struct node;

	/*
	 * Entry holding the value.
	 */
	struct entry {
		entry() : key(0), inode(nullptr), value(nullptr)
		{
		}

		entry(key_type _key, value_type _value)
		    : key(_key), inode(nullptr), value(_value)
		{
		}

		pmem::obj::p<key_type> key;
		pmem::obj::persistent_ptr<node> inode;
		value_type value;

		void
		clear()
		{
			if (inode) {
				inode->clear();
				pmem::obj::delete_persistent<node>(inode);
				inode = nullptr;
			}
			pmem::obj::delete_persistent<T>(value);
			value = nullptr;
		}
	};

	/*
	 * Internal node pointing to two entries.
	 */
	struct node {
		node() : diff(0)
		{
			entries[0] = nullptr;
			entries[1] = nullptr;
		}

		pmem::obj::p<int> diff; /* most significant differing bit */
		pmem::obj::persistent_ptr<entry> entries[2];

		void
		clear()
		{
			if (entries[0]) {
				entries[0]->clear();
				pmem::obj::delete_persistent<entry>(entries[0]);
				entries[0] = nullptr;
			}
			if (entries[1]) {
				entries[1]->clear();
				pmem::obj::delete_persistent<entry>(entries[1]);
				entries[1] = nullptr;
			}
		}
	};

	/*
	 * Find critical bit.
	 */
	static int
	find_crit_bit(key_type lhs, key_type rhs)
	{
		return find_last_set_64(lhs ^ rhs);
	}

	/*
	 * Insert leaf into the tree.
	 */
	void
	insert_leaf(const entry *e, int diff)
	{
		auto new_node = pmem::obj::make_persistent<node>();
		new_node->diff = diff;

		int d = BIT_IS_SET(e->key, new_node->diff);
		new_node->entries[d] = pmem::obj::make_persistent<entry>(*e);

		auto dest_entry = root;
		while (dest_entry->inode != nullptr) {
			auto n = dest_entry->inode;
			if (n->diff < new_node->diff)
				break;

			dest_entry = n->entries[BIT_IS_SET(e->key, n->diff)];
		}

		new_node->entries[!d] =
			pmem::obj::make_persistent<entry>(*dest_entry);
		dest_entry->key = 0;
		dest_entry->inode = new_node;
		dest_entry->value = nullptr;
	}

	/*
	 * Fetch leaf from the tree.
	 */
	pmem::obj::persistent_ptr<entry>
	get_leaf(key_type key, pmem::obj::persistent_ptr<entry> *parent)
	{
		auto n = root;
		pmem::obj::persistent_ptr<entry> p = nullptr;

		while (n->inode != nullptr) {
			p = n;
			n = n->inode->entries[BIT_IS_SET(key, n->inode->diff)];
		}

		if (n->key == key) {
			if (parent)
				*parent = p;

			return n;
		}

		return nullptr;
	}

	/*
	 * Recursive foreach on nodes.
	 */
	int
	foreach_node(const pmem::obj::persistent_ptr<entry> e, callback clb,
		     void *arg)
	{
		int ret = 0;

		if (e->inode != nullptr) {
			auto n = e->inode;
			if (foreach_node(n->entries[0], clb, arg) == 0)
				foreach_node(n->entries[1], clb, arg);
		} else {
			ret = clb(e->key, e->value, arg);
		}

		return ret;
	}

	/* Tree root */
	pmem::obj::persistent_ptr<entry> root;
};

} /* namespace examples */

#endif /* LIBPMEMOBJ_CPP_EXAMPLES_CTREE_MAP_PERSISTENT_HPP */
