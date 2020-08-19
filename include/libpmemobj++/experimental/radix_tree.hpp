// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * @file
 * Implementation of persistent radix tree.
 * Based on: https://github.com/pmem/vmemcache/blob/master/src/critnib.h
 *
 * The implementation is a variation of a PATRICIA trie - the internal
 * nodes do not store the path explicitly, but only a position at which
 * the keys differ. Keys are stored entirely in leaves.
 *
 * More info about radix tree: https://en.wikipedia.org/wiki/Radix_tree
 */

#ifndef LIBPMEMOBJ_CPP_RADIX_HPP
#define LIBPMEMOBJ_CPP_RADIX_HPP

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/container/string.hpp>
#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pext.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/string_view.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/utils.hpp>

#include <algorithm>
#include <iostream>
#include <string>
#if __cpp_lib_endian
#include <bit>
#endif

#include <libpmemobj++/detail/common.hpp>
#include <libpmemobj++/detail/integer_sequence.hpp>

namespace pmem
{

namespace detail
{
template <typename T, typename Enable = void>
struct bytes_view;
}

namespace obj
{

namespace experimental
{

/**
 * Radix tree is an associative, ordered container. Its API is similar
 * to the API of std::map.
 *
 * Unlike std::map radix tree does not use comparison (std::less or equivalent)
 * to locate elements. Instead, keys are mapped to a sequence of bytes using
 * user-provided BytesView type. The key's bytes uniquely define the position of
 * an element. In some way, it is similar to a hash table (BytesView can be
 * treated as a hash function) but with sorted elements.
 *
 * The elements' ordering is defined based on the BytesView mapping. The byte
 * sequences are compared using a function equivalent to std::string::compare.
 *
 * BytesView should accept a pointer to the key type in a constructor and
 * provide operator[] and size method. The declaration should be as following:
 *
 * @code
 * struct BytesView {
 *  BytesView(const Type* t);
 *  const T& operator[](size_t pos) const; // Must be const!
 *  size_t size() const; // Must be const!
 * };
 * @endcode
 *
 * By default, implementation for pmem::obj::inline_string and unsigned integral
 * types is provided. Note that integral types are assumed to be in
 * little-endian.
 *
 * Iterators and references are stable (are not invalidated by inserts or erases
 * of other elements nor by assigning to the value) for all value types except
 * inline_string.
 *
 * In case of inline_string, iterators and references are not invalidated by
 * other inserts or erases, but might be invalidated by assigning new value to
 * the element. Using find(K).assign_val("new_value") may invalidate other
 * iterators and references to the element with key K.
 *
 * swap() invalidates all references and iterators.
 *
 * An example of custom BytesView implementation:
 * @snippet radix_tree/radix_tree_custom_key.cpp bytes_view_example
 */
template <typename Key, typename Value,
	  typename BytesView = detail::bytes_view<Key>>
class radix_tree {
	template <bool IsConst>
	struct radix_tree_iterator;

public:
	using key_type = Key;
	using mapped_type = Value;
	using value_type = std::pair<const key_type, mapped_type>;
	using size_type = std::size_t;
	using const_key_reference = typename std::conditional<
		std::is_same<Key, inline_string>::value, string_view,
		const key_type &>::type;
	using reference = value_type &;
	using const_reference = const value_type &;
	using iterator = radix_tree_iterator<false>;
	using const_iterator = radix_tree_iterator<true>;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	radix_tree();
	template <class InputIt>
	radix_tree(InputIt first, InputIt last);

	// radix_tree(const radix_tree& m);
	// radix_tree(radix_tree&& m);
	// radix_tree(initializer_list<value_type> il);

	// radix_tree& operator=(const radix_tree& m);
	// radix_tree& operator=(radix_tree&& m)

	~radix_tree();

	template <class... Args>
	std::pair<iterator, bool> try_emplace(const_key_reference k,
					      Args &&... args);
	template <class... Args>
	std::pair<iterator, bool> emplace(Args &&... args);

	// pair<iterator, bool> insert(const value_type& v);
	// pair<iterator, bool> insert(value_type&& v);
	// template <class P>
	//     pair<iterator, bool> insert(P&& p);
	// iterator insert(const_iterator position, const value_type& v);
	// iterator insert(const_iterator position, value_type&& v);
	// template <class P>
	//     iterator insert(const_iterator position, P&& p);
	// template <class InputIterator>
	//     void insert(InputIterator first, InputIterator last);
	// void insert(initializer_list<value_type> il);
	// insert_return_type insert(node_type&& nh);
	// iterator insert(const_iterator hint, node_type&& nh);
	// template <class... Args>
	//     pair<iterator, bool> try_emplace(key_type&& k, Args&&... args);
	// template <class... Args>
	//     iterator try_emplace(const_iterator hint, const key_type& k,
	//     Args&&... args);
	// template <class... Args>
	//     iterator try_emplace(const_iterator hint, key_type&& k, Args&&...
	//     args);
	// template <class M>
	//     pair<iterator, bool> insert_or_assign(const key_type& k, M&&
	//     obj);
	// template <class M>
	//     pair<iterator, bool> insert_or_assign(key_type&& k, M&& obj);
	// template <class M>
	//     iterator insert_or_assign(const_iterator hint, const key_type& k,
	//     M&& obj);
	// template <class M>
	//     iterator insert_or_assign(const_iterator hint, key_type&& k, M&&
	//     obj);

	iterator erase(const_iterator pos);
	iterator erase(const_iterator first, const_iterator last);
	size_type erase(const_key_reference k);
	// template <typename K>
	// size_type erase(K&& k);

	void clear();

	size_type count(const_key_reference k) const;
	// template <typename K>
	// size_type count(K&& k) const;

	iterator find(const_key_reference k);
	const_iterator find(const_key_reference k) const;
	// template <typename K>
	// const_iterator find(K&& k) const;

	iterator lower_bound(const_key_reference k);
	const_iterator lower_bound(const_key_reference k) const;
	// template <typename K>
	// const_iterator lower_bound(K&& k) const;

	iterator upper_bound(const_key_reference k);
	const_iterator upper_bound(const_key_reference k) const;
	// template <typename K>
	// const_iterator upper_bound(K&& k) const;

	iterator begin();
	iterator end();
	const_iterator cbegin() const;
	const_iterator cend() const;
	const_iterator begin() const;
	const_iterator end() const;

	reverse_iterator rbegin();
	reverse_iterator rend();
	const_reverse_iterator crbegin() const;
	const_reverse_iterator crend() const;
	const_reverse_iterator rbegin() const;
	const_reverse_iterator rend() const;

	/* capacity: */
	// bool      empty()    const noexcept;
	// size_type max_size() const noexcept;
	uint64_t size() const noexcept;

	void swap(radix_tree &rhs);

	template <typename K, typename V, typename BV>
	friend std::ostream &operator<<(std::ostream &os,
					const radix_tree<K, V, BV> &tree);

private:
	using byten_t = uint32_t;
	using bitn_t = uint8_t;

	/* Size of a chunk which differentiates subtrees of a node */
	static constexpr std::size_t SLICE = 4;
	/* Mask for SLICE */
	static constexpr std::size_t NIB = ((1ULL << SLICE) - 1);
	/* Number of children in internal nodes */
	static constexpr std::size_t SLNODES = (1 << SLICE);
	/* Mask for SLICE */
	static constexpr bitn_t SLICE_MASK = (bitn_t) ~(SLICE - 1);
	/* Position of the first SLICE */
	static constexpr bitn_t FIRST_NIB = 8 - SLICE;

	struct tagged_node_ptr;
	struct leaf;
	struct node;

	/*** pmem members ***/
	tagged_node_ptr root;
	p<uint64_t> size_;

	/* helper functions */
	template <typename F, class... Args>
	std::pair<iterator, bool> internal_emplace(const_key_reference k, F &&);
	template <typename K>
	leaf *internal_find(const K &k) const;

	static tagged_node_ptr &parent_ref(tagged_node_ptr n);
	template <typename K1, typename K2>
	static bool keys_equal(const K1 &k1, const K2 &k2);
	template <typename K1, typename K2>
	static int compare(const K1 &k1, const K2 &k2);
	template <bool Direction, typename Iterator>
	static leaf *next_leaf(Iterator child, tagged_node_ptr parent);
	template <bool Direction>
	static leaf *find_leaf(tagged_node_ptr n);
	static unsigned slice_index(char k, uint8_t shift);
	template <typename K1, typename K2>
	static byten_t prefix_diff(const K1 &lhs, const K2 &rhs);
	leaf *bottom_leaf(tagged_node_ptr n) const;
	template <typename K>
	leaf *descend(const K &key) const;
	static void print_rec(std::ostream &os, radix_tree::tagged_node_ptr n);
	template <typename K>
	static BytesView bytes_view(const K &k);
	static string_view bytes_view(string_view s);
	static bool path_length_equal(size_t key_size, tagged_node_ptr n);
	void check_pmem();
	void check_tx_stage_work();

	static_assert(sizeof(node) == 256,
		      "Internal node should have size equal to 256 bytes.");
};

template <typename Key, typename Value, typename BytesView>
void swap(radix_tree<Key, Value, BytesView> &lhs,
	  radix_tree<Key, Value, BytesView> &rhs);

template <typename Key, typename Value, typename BytesView>
struct radix_tree<Key, Value, BytesView>::tagged_node_ptr {
	tagged_node_ptr();
	tagged_node_ptr(const tagged_node_ptr &rhs);
	tagged_node_ptr(std::nullptr_t);

	tagged_node_ptr(const persistent_ptr<leaf> &ptr);
	tagged_node_ptr(const persistent_ptr<node> &ptr);

	tagged_node_ptr &operator=(const tagged_node_ptr &rhs);
	tagged_node_ptr &operator=(std::nullptr_t);
	tagged_node_ptr &operator=(const persistent_ptr<leaf> &rhs);
	tagged_node_ptr &operator=(const persistent_ptr<node> &rhs);

	bool operator==(const tagged_node_ptr &rhs) const;
	bool operator!=(const tagged_node_ptr &rhs) const;

	bool operator==(const radix_tree::leaf *rhs) const;
	bool operator!=(const radix_tree::leaf *rhs) const;

	void swap(tagged_node_ptr &rhs);

	bool is_leaf() const;

	radix_tree::leaf *get_leaf() const;
	radix_tree::node *get_node() const;

	radix_tree::node *operator->() const noexcept;

	explicit operator bool() const noexcept;

private:
	static constexpr unsigned IS_LEAF = 1;

	uint64_t get() const noexcept;

	p<uint64_t> off;
};

/**
 * This is the structure which 'holds' key/value pair. The data
 * is not stored as an object within this structure but rather
 * just after the structure (using emplace new). This is done
 * so that we can use inline_string and limit the number of allocations.
 *
 * Constructors of the leaf structure mimics those of std::pair<const Key,
 * Value>.
 */
template <typename Key, typename Value, typename BytesView>
struct radix_tree<Key, Value, BytesView>::leaf {
	using tree_type = radix_tree<Key, Value, BytesView>;

	leaf(const leaf &) = delete;
	leaf(leaf &&) = delete;

	leaf &operator=(const leaf &) = delete;
	leaf &operator=(leaf &&) = delete;

	~leaf();

	Key &key();
	Value &value();

	const Key &key() const;
	const Value &value() const;

private:
	friend class radix_tree<Key, Value, BytesView>;

	leaf() = default;

	template <typename... Args>
	static persistent_ptr<leaf> make(tagged_node_ptr parent,
					 Args &&... args);

	static persistent_ptr<leaf> make_internal();

	template <typename... Args1, typename... Args2>
	static persistent_ptr<leaf>
	make_internal(std::piecewise_construct_t pc,
		      std::tuple<Args1...> first_args,
		      std::tuple<Args2...> second_args);

	template <typename K, typename V>
	static persistent_ptr<leaf> make_internal(K &&k, V &&v);
	template <typename K, typename V>
	static persistent_ptr<leaf> make_internal(const K &k, const V &v);

	template <typename K, typename V>
	static persistent_ptr<leaf> make_internal(std::pair<K, V> &&p);
	template <typename K, typename V>
	static persistent_ptr<leaf> make_internal(const std::pair<K, V> &p);

	template <typename... Args1, typename... Args2, size_t... I1,
		  size_t... I2>
	static persistent_ptr<leaf> make_internal(
		std::piecewise_construct_t, std::tuple<Args1...> &first_args,
		std::tuple<Args2...> &second_args,
		detail::index_sequence<I1...>, detail::index_sequence<I2...>);

	tagged_node_ptr parent = nullptr;
};

/**
 * This is internal node. It does not hold any values directly, but
 * can contain pointer to an embedded entry (see below).
 */
template <typename Key, typename Value, typename BytesView>
struct radix_tree<Key, Value, BytesView>::node {
	node(tagged_node_ptr parent, byten_t byte, bitn_t bit);

	/**
	 * Pointer to a parent node. Used by iterators.
	 */
	tagged_node_ptr parent;

	/**
	 * The embedded_entry ptr is used only for nodes for which length of the
	 * path from root is a multiple of byte (n->bit == FIRST_NIB). This
	 * entry holds a key which represents the entire subtree prefix (path
	 * from root).
	 */
	tagged_node_ptr embedded_entry;

	/* Children can be both leaves and internal nodes. */
	tagged_node_ptr child[SLNODES];

	/**
	 * Byte and bit together are used to calculate the NIB which is used to
	 * index the child array. The calculations are done in slice_index
	 * function.
	 *
	 * Let's say we have a key = 0xABCD
	 *
	 * For byte = 0, bit = 4 we have NIB = 0xA
	 * For byte = 0, bit = 0 we have NIB = 0xB
	 */
	byten_t byte;
	bitn_t bit;

	struct direction {
		static constexpr bool Forward = 0;
		static constexpr bool Reverse = 1;
	};

	struct forward_iterator;
	using reverse_iterator = std::reverse_iterator<forward_iterator>;

	template <bool Direction>
	using iterator =
		typename std::conditional<Direction == direction::Forward,
					  forward_iterator,
					  reverse_iterator>::type;

	template <bool Direction = direction::Forward>
	typename std::enable_if<
		Direction ==
			radix_tree<Key, Value,
				   BytesView>::node::direction::Forward,
		typename radix_tree<Key, Value,
				    BytesView>::node::forward_iterator>::type
	begin() const;

	template <bool Direction = direction::Forward>
	typename std::enable_if<
		Direction ==
			radix_tree<Key, Value,
				   BytesView>::node::direction::Forward,
		typename radix_tree<Key, Value,
				    BytesView>::node::forward_iterator>::type
	end() const;

	/* rbegin */
	template <bool Direction = direction::Forward>
	typename std::enable_if<
		Direction ==
			radix_tree<Key, Value,
				   BytesView>::node::direction::Reverse,
		typename radix_tree<Key, Value,
				    BytesView>::node::reverse_iterator>::type
	begin() const;

	/* rend */
	template <bool Direction = direction::Forward>
	typename std::enable_if<
		Direction ==
			radix_tree<Key, Value,
				   BytesView>::node::direction::Reverse,
		typename radix_tree<Key, Value,
				    BytesView>::node::reverse_iterator>::type
	end() const;

	template <bool Direction = direction::Forward, typename Ptr>
	auto find_child(const Ptr &n) const -> decltype(begin<Direction>());

	template <bool Direction = direction::Forward,
		  typename Enable = typename std::enable_if<
			  Direction == direction::Forward>::type>
	auto make_iterator(const tagged_node_ptr *ptr) const
		-> decltype(begin<Direction>());

	uint8_t padding[256 - sizeof(parent) - sizeof(leaf) - sizeof(child) -
			sizeof(byte) - sizeof(bit)];
};

/**
 * Radix tree iterator supports multipass and bidirectional iteration.
 * If Value type is inline_string, calling (*it).second = "new_value"
 * might cause reallocation and invalidate iterators to that element.
 */
template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
struct radix_tree<Key, Value, BytesView>::radix_tree_iterator {
private:
	using leaf_ptr =
		typename std::conditional<IsConst, const leaf *, leaf *>::type;
	using node_ptr =
		typename std::conditional<IsConst, const tagged_node_ptr *,
					  tagged_node_ptr *>::type;
	friend struct radix_tree_iterator<true>;

public:
	using difference_type = std::ptrdiff_t;
	using value_type = radix_tree::leaf;
	using reference = typename std::conditional<IsConst, const value_type &,
						    value_type &>::type;
	using pointer = typename std::conditional<IsConst, value_type const *,
						  value_type *>::type;
	using iterator_category = std::bidirectional_iterator_tag;

	radix_tree_iterator() = default;
	radix_tree_iterator(leaf_ptr leaf_, node_ptr root);
	radix_tree_iterator(const radix_tree_iterator &rhs) = default;

	template <bool C = IsConst,
		  typename Enable = typename std::enable_if<C>::type>
	radix_tree_iterator(const radix_tree_iterator<false> &rhs);

	radix_tree_iterator &
	operator=(const radix_tree_iterator &rhs) = default;

	reference operator*() const;
	pointer operator->() const;

	template <typename V = Value,
		  typename Enable = typename std::enable_if<
			  std::is_same<V, inline_string>::value>::type>
	void assign_val(string_view rhs);

	template <typename T, typename V = Value,
		  typename Enable = typename std::enable_if<
			  !std::is_same<V, inline_string>::value>::type>
	void assign_val(T &&rhs);

	radix_tree_iterator &operator++();
	radix_tree_iterator &operator--();

	// XXX:
	// radix_tree_iterator operator++(int);
	// radix_tree_iterator operator--(int);

	template <bool C>
	bool operator!=(const radix_tree_iterator<C> &rhs) const;

	template <bool C>
	bool operator==(const radix_tree_iterator<C> &rhs) const;

private:
	friend class radix_tree;

	leaf_ptr leaf_ = nullptr;
	node_ptr root = nullptr;
};

template <typename Key, typename Value, typename BytesView>
struct radix_tree<Key, Value, BytesView>::node::forward_iterator {
	using difference_type = std::ptrdiff_t;
	using value_type = tagged_node_ptr;
	using pointer = const value_type *;
	using reference = const value_type &;
	using iterator_category = std::forward_iterator_tag;

	forward_iterator(pointer child, const node *node);

	forward_iterator operator++();
	forward_iterator operator++(int);

	forward_iterator operator--();

	reference operator*() const;
	pointer operator->() const;

	pointer get_node() const;

	bool operator!=(const forward_iterator &rhs) const;
	bool operator==(const forward_iterator &rhs) const;

private:
	pointer child;
	const node *n;
};

/**
 * Default radix tree constructor. Constructs an empty container.
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::radix_tree() : root(nullptr), size_(0)
{
	check_pmem();
	check_tx_stage_work();
}

/**
 * Constructs the container with the contents of the range [first,
 * last). If multiple elements in the range have keys that compare
 * equivalent, the first element is inserted.
 *
 * @param[in] first first iterator of inserted range.
 * @param[in] last last iterator of inserted range.
 *
 * InputIt must meet the requirements of LegacyInputIterator.
 *
 * @pre must be called in transaction scope.
 *
 * @throw pmem::pool_error if an object is not in persistent memory.
 * @throw pmem::transaction_scope_error if constructor wasn't called in
 * transaction.
 * @throw pmem::transaction_alloc_error when allocating memory for
 * inserted elements in transaction failed.
 * @throw rethrows element constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <class InputIt>
radix_tree<Key, Value, BytesView>::radix_tree(InputIt first, InputIt last)
    : root(nullptr), size_(0)
{
	check_pmem();
	check_tx_stage_work();

	for (auto it = first; it != last; it++)
		emplace(*it);
}

/**
 * Destructor.
 */
template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::~radix_tree()
{
	try {
		clear();
	} catch (...) {
		std::terminate();
	}
}

/**
 * @return number of elements.
 */
template <typename Key, typename Value, typename BytesView>
uint64_t
radix_tree<Key, Value, BytesView>::size() const noexcept
{
	return this->size_;
}

/**
 * Member swap.
 *
 * Exchanges *this with @param rhs
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::swap(radix_tree &rhs)
{
	auto pop = pool_by_vptr(this);

	transaction::run(pop, [&] {
		this->size_.swap(rhs.size_);
		this->root.swap(rhs.root);
	});
}

/*
 * Find a bottom (leftmost) leaf in a subtree.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::bottom_leaf(
	typename radix_tree<Key, Value, BytesView>::tagged_node_ptr n) const
{
	assert(n);

	while (!n.is_leaf()) {
		for (size_t i = 0; i < SLNODES; i++) {
			tagged_node_ptr m;
			if ((m = n->child[i])) {
				n = m;
				break;
			}
		}
	}

	return n.get_leaf();
}

/*
 * Returns reference to n->parent (handles both internal and leaf nodes).
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::tagged_node_ptr &
radix_tree<Key, Value, BytesView>::parent_ref(tagged_node_ptr n)
{
	if (n.is_leaf())
		return n.get_leaf()->parent;

	return n->parent;
}

/*
 * Descends to the leaf that shares a common prefix with the key.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::descend(const K &key) const
{
	auto n = root;

	while (!n.is_leaf() && n->byte < key.size()) {
		auto nn = n->child[slice_index(key[n->byte], n->bit)];

		if (nn)
			n = nn;
		else {
			n = bottom_leaf(n);
			break;
		}
	}

	/* This can happen when key is a prefix of some leaf or when the node at
	 * which the keys diverge isn't a leaf */
	if (!n.is_leaf())
		n = bottom_leaf(n);

	return n.get_leaf();
}

template <typename Key, typename Value, typename BytesView>
template <typename K>
BytesView
radix_tree<Key, Value, BytesView>::bytes_view(const K &key)
{
	/* bytes_view accepts const pointer instead of reference to make sure
	 * there is no implicit conversion to a temporary type (and hence
	 * dangling references). */
	return BytesView(&key);
}

template <typename Key, typename Value, typename BytesView>
string_view
radix_tree<Key, Value, BytesView>::bytes_view(string_view key)
{
	return key;
}

/*
 * Checks for key equality.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K1, typename K2>
bool
radix_tree<Key, Value, BytesView>::keys_equal(const K1 &k1, const K2 &k2)
{
	return k1.size() == k2.size() && compare(k1, k2) == 0;
}

/*
 * Checks for key equality.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K1, typename K2>
int
radix_tree<Key, Value, BytesView>::compare(const K1 &k1, const K2 &k2)
{
	auto ret = prefix_diff(k1, k2);

	if (ret != (std::min)(k1.size(), k2.size()))
		return (unsigned char)(k1[ret]) - (unsigned char)(k2[ret]);

	return k1.size() - k2.size();
}

/*
 * Returns length of common prefix of lhs and rhs.
 */
template <typename Key, typename Value, typename BytesView>
template <typename K1, typename K2>
typename radix_tree<Key, Value, BytesView>::byten_t
radix_tree<Key, Value, BytesView>::prefix_diff(const K1 &lhs, const K2 &rhs)
{
	byten_t diff;
	for (diff = 0; diff < (std::min)(lhs.size(), rhs.size()); diff++) {
		if (lhs[diff] != rhs[diff])
			return diff;
	}

	return diff;
}

/*
 * Checks whether length of the path from root to n is equal
 * to key_size.
 */
template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::path_length_equal(size_t key_size,
						     tagged_node_ptr n)
{
	return n->byte == key_size && n->bit == bitn_t(FIRST_NIB);
}

template <typename Key, typename Value, typename BytesView>
template <typename F, class... Args>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::internal_emplace(const_key_reference k,
						    F &&make_leaf)
{
	auto key = bytes_view(k);
	auto pop = pool_base(pmemobj_pool_by_ptr(this));

	if (!root) {
		transaction::run(pop, [&] { this->root = make_leaf(nullptr); });
		return {iterator(root.get_leaf(), &root), true};
	}

	/*
	 * Need to descend the tree twice. First to find a leaf that
	 * represents a subtree that shares a common prefix with the key.
	 * This is needed to find out the actual labels between nodes (they
	 * are not known due to a possible path compression). Second time to
	 * find the place for the new element.
	 */
	auto leaf = descend(key);
	auto leaf_key = bytes_view(leaf->key());
	auto diff = prefix_diff(key, leaf_key);

	auto n = root;
	auto child_slot = &root;
	auto prev = n;

	auto min_key_len = (std::min)(leaf_key.size(), key.size());
	bitn_t sh = 8;

	/* If key differs from leaf_key at some point (neither is a prefix of
	 * another) we will descend to the point of divergence. Otherwise we
	 * will look for a node which represents the prefix. */
	if (diff < min_key_len) {
		auto at =
			static_cast<unsigned char>(leaf_key[diff] ^ key[diff]);
		sh = pmem::detail::mssb_index((uint32_t)at) & SLICE_MASK;
	}

	/* Descend into the tree again. */
	while (n && !n.is_leaf() &&
	       (n->byte < diff || (n->byte == diff && n->bit >= sh))) {
		prev = n;
		child_slot = &n->child[slice_index(key[n->byte], n->bit)];
		n = *child_slot;
	}

	/*
	 * If the divergence point is at same nib as an existing node, and
	 * the subtree there is empty, just place our leaf there and we're
	 * done.  Obviously this can't happen if SLICE == 1.
	 */
	if (!n) {
		assert(diff < min_key_len);

		transaction::run(pop, [&] { *child_slot = make_leaf(prev); });
		return {iterator(child_slot->get_leaf(), &root), true};
	}

	/* New key is a prefix of the leaf key or they are equal. We need to add
	 * leaf ptr to internal node. */
	if (diff == key.size()) {
		// XXX we can just return leaf before the second loop.
		if (n.is_leaf() &&
		    bytes_view(n.get_leaf()->key()).size() == key.size()) {
			/* Key exists. */
			return {iterator(child_slot->get_leaf(), &root), false};
		}

		if (!n.is_leaf() && path_length_equal(key.size(), n)) {
			if (n->embedded_entry)
				return {iterator(n->embedded_entry.get_leaf(),
						 &root),
					false};

			transaction::run(
				pop, [&] { n->embedded_entry = make_leaf(n); });

			return {iterator(n->embedded_entry.get_leaf(), &root),
				true};
		}

		/* Path length from root to n is longer than key.size().
		 * We have to allocate new internal node above n. */
		tagged_node_ptr node;
		transaction::run(pop, [&] {
			node = make_persistent<radix_tree::node>(
				parent_ref(n), diff, bitn_t(FIRST_NIB));
			node->embedded_entry = make_leaf(node);
			node->child[slice_index(leaf_key[diff],
						bitn_t(FIRST_NIB))] = n;

			parent_ref(n) = node;
			*child_slot = node;
		});

		return {iterator(node->embedded_entry.get_leaf(), &root), true};
	}

	if (diff == leaf_key.size()) {
		/* Leaf key is a prefix of the new key. We need to convert leaf
		 * to a node. */
		tagged_node_ptr node;
		transaction::run(pop, [&] {
			/* We have to add new node at the edge from parent to n
			 */
			node = make_persistent<radix_tree::node>(
				parent_ref(n), diff, bitn_t(FIRST_NIB));
			node->embedded_entry = n;
			node->child[slice_index(key[diff], bitn_t(FIRST_NIB))] =
				make_leaf(node);

			parent_ref(n) = node;
			*child_slot = node;
		});

		return {iterator(node->child[slice_index(key[diff],
							 bitn_t(FIRST_NIB))]
					 .get_leaf(),
				 &root),
			true};
	}

	/* There is already a subtree at the divergence point
	 * (slice_index(key[diff], sh)). This means that a tree is vertically
	 * compressed and we have to "break" this compression and add a new
	 * node. */
	tagged_node_ptr node;
	transaction::run(pop, [&] {
		node = make_persistent<radix_tree::node>(parent_ref(n), diff,
							 sh);
		node->child[slice_index(leaf_key[diff], sh)] = n;
		node->child[slice_index(key[diff], sh)] = make_leaf(node);

		parent_ref(n) = node;
		*child_slot = node;
	});

	return {iterator(node->child[slice_index(key[diff], sh)].get_leaf(),
			 &root),
		true};
}

/**
 * If a key equivalent to k already exists in the container, does
 * nothing. Otherwise, behaves like emplace except that the element is
 * constructed as value_type(std::piecewise_construct,
 * std::forward_as_tuple(k),
 * std::forward_as_tuple(std::forward<Args>(args)...))
 *
 * No iterators or references are invalidated.
 *
 * @param[in] k the key used both to look up and to insert if not found.
 * @param[in] args arguments to forward to the constructor of the
 * element.
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <class... Args>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::try_emplace(const_key_reference k,
					       Args &&... args)
{
	return internal_emplace(k, [&](tagged_node_ptr parent) {
		size_++;
		return leaf::make(parent, k, std::forward<Args>(args)...);
	});
}

/**
 * Inserts a new element into the container constructed in-place with
 * the given args if there is no element with the key in the container.
 *
 * Careful use of emplace allows the new element to be constructed while
 * avoiding unnecessary copy or move operations. The constructor of the
 * new element (i.e. std::pair<const Key, T>) is called with exactly the
 * same arguments as supplied to emplace, forwarded via
 * std::forward<Args>(args).... The element may be constructed even if
 * there already is an element with the key in the container, in which
 * case the newly constructed element will be destroyed immediately.
 *
 * No iterators or references are invalidated.
 *
 * @param[in] args arguments to forward to the constructor of the
 * element
 *
 * @return a pair consisting of an iterator to the inserted element (or
 * to the element that prevented the insertion) and a bool denoting
 * whether the insertion took place.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw pmem::transaction_alloc_error when allocating new memory
 * failed.
 * @throw pmem::transaction_scope_error if called inside transaction.
 * @throw rethrows constructor exception.
 */
template <typename Key, typename Value, typename BytesView>
template <class... Args>
std::pair<typename radix_tree<Key, Value, BytesView>::iterator, bool>
radix_tree<Key, Value, BytesView>::emplace(Args &&... args)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(this));
	std::pair<iterator, bool> ret;

	transaction::run(pop, [&] {
		auto leaf_ = leaf::make(nullptr, std::forward<Args>(args)...);
		auto make_leaf = [&](tagged_node_ptr parent) {
			leaf_->parent = parent;
			size_++;
			return leaf_;
		};

		ret = internal_emplace(leaf_->key(), make_leaf);

		if (!ret.second)
			delete_persistent<leaf>(leaf_);
	});

	return ret;
}

/**
 * Returns the number of elements with key that compares equivalent to
 * the specified argument.
 *
 * @param[in] k key value of the element to count.
 *
 * @return Number of elements with key that compares equivalent to the
 * specified argument.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::size_type
radix_tree<Key, Value, BytesView>::count(const_key_reference k) const
{
	return internal_find(k) != nullptr ? 1 : 0;
}

/**
 * Finds an element with key equivalent to key.
 *
 * @param[in] k key value of the element to search for.
 *
 * @return Iterator to an element with key equivalent to key. If no such
 * element is found, past-the-end iterator is returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::find(const_key_reference k)
{
	return iterator(internal_find(k), &root);
}

/**
 * Finds an element with key equivalent to key.
 *
 * @param[in] k key value of the element to search for.
 *
 * @return Iterator to an element with key equivalent to key. If no such
 * element is found, past-the-end iterator is returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::find(const_key_reference k) const
{
	return const_iterator(internal_find(k), &root);
}

template <typename Key, typename Value, typename BytesView>
template <typename K>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::internal_find(const K &k) const
{
	auto key = bytes_view(k);

	auto n = root;
	while (n && !n.is_leaf()) {
		if (path_length_equal(key.size(), n))
			n = n->embedded_entry;
		else if (n->byte > key.size())
			return nullptr;
		else
			n = n->child[slice_index(key[n->byte], n->bit)];
	}

	if (!n)
		return nullptr;

	if (!keys_equal(key, bytes_view(n.get_leaf()->key())))
		return nullptr;

	return n.get_leaf();
}

/**
 * Erases all elements from the container transactionally.
 *
 * @post size() == 0
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::clear()
{
	if (size() != 0)
		erase(begin(), end());
}

/**
 * Removes the element at pos from the container.
 * References and iterators to the erased elements are invalidated.
 * Other references and iterators are not affected.
 *
 * @pre The iterator pos must be valid and dereferenceable. Thus the
 * end() iterator (which is valid, but is not dereferenceable) cannot be
 * used as a value for pos.
 *
 * @param[in] pos iterator to the element to remove.
 *
 * @return iterator following the removed element.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::erase(const_iterator pos)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(this));

	transaction::run(pop, [&] {
		auto *leaf = pos.leaf_;
		auto parent = leaf->parent;

		delete_persistent<radix_tree::leaf>(
			persistent_ptr<radix_tree::leaf>(leaf));

		size_--;

		/* was root */
		if (!parent) {
			this->root = nullptr;
			pos = begin();
			return;
		}

		++pos;

		/* It's safe to cast because we're inside non-const method. */
		const_cast<tagged_node_ptr &>(*parent->find_child(leaf)) =
			nullptr;

		/* Compress the tree vertically. */
		auto n = parent;
		parent = n->parent;
		tagged_node_ptr only_child = nullptr;
		for (size_t i = 0; i < SLNODES; i++) {
			if (n->child[i]) {
				if (only_child) {
					/* more than one child */
					return;
				}
				only_child = n->child[i];
			}
		}

		if (only_child && n->embedded_entry) {
			/* There are actually 2 "children" so we can't compress.
			 */
			return;
		} else if (n->embedded_entry) {
			only_child = n->embedded_entry;
		}

		assert(only_child);
		parent_ref(only_child) = n->parent;

		auto *child_slot = parent
			? const_cast<tagged_node_ptr *>(&*parent->find_child(n))
			: &root;
		*child_slot = only_child;

		delete_persistent<radix_tree::node>(n.get_node());
	});

	return iterator(const_cast<typename iterator::leaf_ptr>(pos.leaf_),
			&root);
}

/**
 * Removes the elements in the range [first; last), which must be a
 * valid range in *this.
 * References and iterators to the erased elements are invalidated.
 * Other references and iterators are not affected.
 *
 * @param[in] first first iterator in the range of elements to remove.
 * @param[in] last last iterator in the range of elements to remove.
 *
 * @return iterator following the last removed element.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::erase(const_iterator first,
					 const_iterator last)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(this));

	transaction::run(pop, [&] {
		while (first != last)
			first = erase(first);
	});

	return iterator(const_cast<typename iterator::leaf_ptr>(first.leaf_),
			&root);
}

/**
 * Removes the element (if one exists) with the key equivalent to key.
 * References and iterators to the erased elements are invalidated.
 * Other references and iterators are not affected.
 *
 * @param[in] k key value of the elements to remove.
 *
 * @return Number of elements removed.
 *
 * @throw pmem::transaction_error when snapshotting failed.
 * @throw rethrows destructor exception.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::size_type
radix_tree<Key, Value, BytesView>::erase(const_key_reference k)
{
	auto it = find(k);

	if (it == end())
		return 0;

	erase(it);

	return 1;
}

/**
 * Returns an iterator pointing to the first element that is not less
 * than (i.e. greater or equal to) key.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Iterator pointing to the first element that is not less than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::lower_bound(const_key_reference k) const
{
	// XXX: optimize this function (todo: refactor out emplace code)

	if (!root)
		return end();

	auto key = bytes_view(k);
	auto leaf = descend(key);

	auto it = const_iterator(leaf, &root);

	/* Iterate until greater or equal element is found. */
	while (it != end() && compare(bytes_view(it->key()), key) < 0)
		++it;

	if (it == end())
		return end();

	/* Find the *first* element greater or equal to key. */
	auto tmp = it;

	if (it == begin())
		return begin();

	--tmp;
	while (it != begin() && compare(bytes_view(tmp->key()), key) >= 0) {
		it = tmp;
		--tmp;
	}

	return it;
}

/**
 * Returns an iterator pointing to the first element that is not less
 * than (i.e. greater or equal to) key.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Iterator pointing to the first element that is not less than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::lower_bound(const_key_reference k)
{
	auto it = const_cast<const radix_tree *>(this)->lower_bound(k);
	return iterator(const_cast<typename iterator::leaf_ptr>(it.leaf_),
			&root);
}

/**
 * Returns an iterator pointing to the first element that is greater
 * than key.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Iterator pointing to the first element that is greater than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::upper_bound(const_key_reference k) const
{
	auto key = bytes_view(k);
	auto it = lower_bound(k);

	if (it == end())
		return end();

	// XXX: optimize -> it's 2 comparisons
	if (keys_equal(bytes_view(it->key()), key))
		++it;

	return it;
}

/**
 * Returns an iterator pointing to the first element that is greater
 * than key.
 *
 * @param[in] k key value to compare the elements to.
 *
 * @return Iterator pointing to the first element that is greater than
 * key. If no such element is found, a past-the-end iterator is
 * returned.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::upper_bound(const_key_reference k)
{
	auto it = const_cast<const radix_tree *>(this)->upper_bound(k);
	return iterator(const_cast<typename iterator::leaf_ptr>(it.leaf_),
			&root);
}

/**
 * Returns an iterator to the first element of the container.
 * If the map is empty, the returned iterator will be equal to end().
 *
 * @return Iterator to the first element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::begin()
{
	auto const_begin = const_cast<const radix_tree *>(this)->begin();
	return iterator(
		const_cast<typename iterator::leaf_ptr>(const_begin.leaf_),
		&root);
}

/**
 * Returns an iterator to the element following the last element of the
 * map. This element acts as a placeholder; attempting to access it
 * results in undefined behavior.
 *
 * @return Iterator to the element following the last element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::iterator
radix_tree<Key, Value, BytesView>::end()
{
	auto const_end = const_cast<const radix_tree *>(this)->end();
	return iterator(
		const_cast<typename iterator::leaf_ptr>(const_end.leaf_),
		&root);
}

/**
 * Returns a const iterator to the first element of the container.
 * If the map is empty, the returned iterator will be equal to end().
 *
 * @return const iterator to the first element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::cbegin() const
{
	if (!root)
		return const_iterator(nullptr, &root);

	return const_iterator(
		radix_tree::find_leaf<radix_tree::node::direction::Forward>(
			root),
		&root);
}

/**
 * Returns a const iterator to the element following the last element of the
 * map. This element acts as a placeholder; attempting to access it
 * results in undefined behavior.
 *
 * @return const iterator to the element following the last element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::cend() const
{
	return const_iterator(nullptr, &root);
}

/**
 * Returns a const iterator to the first element of the container.
 * If the map is empty, the returned iterator will be equal to end().
 *
 * @return const iterator to the first element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::begin() const
{
	return cbegin();
}

/**
 * Returns a const iterator to the element following the last element of the
 * map. This element acts as a placeholder; attempting to access it
 * results in undefined behavior.
 *
 * @return const iterator to the element following the last element.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_iterator
radix_tree<Key, Value, BytesView>::end() const
{
	return cend();
}

/**
 * Returns a reverse iterator to the beginning.
 *
 * @return reverse_iterator pointing to the last element in the vector.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::reverse_iterator
radix_tree<Key, Value, BytesView>::rbegin()
{
	return reverse_iterator(end());
}

/**
 * Returns a reverse iterator to the end.
 *
 * @return reverse_iterator pointing to the theoretical element preceding the
 * first element in the vector.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::reverse_iterator
radix_tree<Key, Value, BytesView>::rend()
{
	return reverse_iterator(begin());
}

/**
 * Returns a const, reverse iterator to the beginning.
 *
 * @return const_reverse_iterator pointing to the last element in the vector.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_reverse_iterator
radix_tree<Key, Value, BytesView>::crbegin() const
{
	return const_reverse_iterator(cend());
}

/**
 * Returns a const, reverse iterator to the end.
 *
 * @return const_reverse_iterator pointing to the theoretical element preceding
 * the first element in the vector.
 */
template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_reverse_iterator
radix_tree<Key, Value, BytesView>::crend() const
{
	return const_reverse_iterator(cbegin());
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_reverse_iterator
radix_tree<Key, Value, BytesView>::rbegin() const
{
	return const_reverse_iterator(cend());
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::const_reverse_iterator
radix_tree<Key, Value, BytesView>::rend() const
{
	return const_reverse_iterator(cbegin());
}

template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::print_rec(std::ostream &os,
					     radix_tree::tagged_node_ptr n)
{
	if (!n.is_leaf()) {
		os << "\"" << n.get_node() << "\""
		   << " [style=filled,color=\"blue\"]" << std::endl;
		os << "\"" << n.get_node() << "\" [label=\"byte:" << n->byte
		   << ", bit:" << int(n->bit) << "\"]" << std::endl;

		auto parent = n->parent ? n->parent.get_node() : 0;
		os << "\"" << n.get_node() << "\" -> "
		   << "\"" << parent << "\" [label=\"parent\"]" << std::endl;

		for (auto it = n->begin(); it != n->end(); ++it) {
			if (!(*it))
				continue;

			auto ch = it->is_leaf() ? (void *)it->get_leaf()
						: (void *)it->get_node();

			os << "\"" << n.get_node() << "\" -> \"" << ch << "\""
			   << std::endl;
			print_rec(os, *it);
		}
	} else {
		auto bv = bytes_view(n.get_leaf()->key());

		os << "\"" << n.get_leaf()
		   << "\" [style=filled,color=\"green\"]" << std::endl;
		os << "\"" << n.get_leaf() << "\" [label=\"key:";

		for (size_t i = 0; i < bv.size(); i++)
			os << bv[i];

		os << "\"]" << std::endl;

		auto parent = n.get_leaf()->parent
			? n.get_leaf()->parent.get_node()
			: nullptr;

		os << "\"" << n.get_leaf() << "\" -> \"" << parent
		   << "\" [label=\"parent\"]" << std::endl;

		if (parent && n == parent->embedded_entry) {
			os << "{rank=same;\"" << parent << "\";\""
			   << n.get_leaf() << "\"}" << std::endl;
		}
	}
}

/**
 * Prints tree in DOT format. Used for debugging.
 */
template <typename K, typename V, typename BV>
std::ostream &
operator<<(std::ostream &os, const radix_tree<K, V, BV> &tree)
{
	os << "digraph Radix {" << std::endl;

	if (tree.root)
		radix_tree<K, V, BV>::print_rec(os, tree.root);

	os << "}" << std::endl;

	return os;
}

/*
 * internal: slice_index -- return index of child at the given nib
 */
template <typename Key, typename Value, typename BytesView>
unsigned
radix_tree<Key, Value, BytesView>::slice_index(char b, uint8_t bit)
{
	return static_cast<unsigned>(b >> bit) & NIB;
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::tagged_node_ptr::tagged_node_ptr(
	std::nullptr_t)
{
	this->off = 0;
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::tagged_node_ptr::tagged_node_ptr()
{
	this->off = 0;
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::tagged_node_ptr::tagged_node_ptr(
	const tagged_node_ptr &rhs)
{
	if (!rhs) {
		this->off = 0;
	} else {
		this->off = rhs.get() - reinterpret_cast<uint64_t>(this);
		off |= unsigned(rhs.is_leaf());
	}
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::tagged_node_ptr &
radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator=(
	const tagged_node_ptr &rhs)
{
	if (!rhs) {
		this->off = 0;
	} else {
		this->off = rhs.get() - reinterpret_cast<uint64_t>(this);
		off |= unsigned(rhs.is_leaf());
	}

	return *this;
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::tagged_node_ptr::tagged_node_ptr(
	const persistent_ptr<leaf> &ptr)
{
	if (!ptr) {
		this->off = 0;
	} else {
		off = reinterpret_cast<uint64_t>(ptr.get()) -
			reinterpret_cast<uint64_t>(this);
		off |= unsigned(IS_LEAF);
	}
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::tagged_node_ptr::tagged_node_ptr(
	const persistent_ptr<node> &ptr)
{
	if (!ptr) {
		this->off = 0;
	} else {
		off = reinterpret_cast<uint64_t>(ptr.get()) -
			reinterpret_cast<uint64_t>(this);
	}
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::tagged_node_ptr &
	radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator=(
		std::nullptr_t)
{
	this->off = 0;

	return *this;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::tagged_node_ptr &
radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator=(
	const persistent_ptr<leaf> &rhs)
{
	if (!rhs) {
		this->off = 0;
	} else {
		off = reinterpret_cast<uint64_t>(rhs.get()) -
			reinterpret_cast<uint64_t>(this);
		off |= unsigned(IS_LEAF);
	}

	return *this;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::tagged_node_ptr &
radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator=(
	const persistent_ptr<node> &rhs)
{
	if (!rhs) {
		this->off = 0;
	} else {
		off = reinterpret_cast<uint64_t>(rhs.get()) -
			reinterpret_cast<uint64_t>(this);
	}

	return *this;
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator==(
	const radix_tree::tagged_node_ptr &rhs) const
{
	return get() == rhs.get() || (!*this && !rhs);
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator!=(
	const radix_tree::tagged_node_ptr &rhs) const
{
	return !(*this == rhs);
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator==(
	const radix_tree::leaf *rhs) const
{
	return is_leaf() && get_leaf() == rhs;
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator!=(
	const radix_tree::leaf *rhs) const
{
	return !(*this == rhs);
}

template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::tagged_node_ptr::swap(tagged_node_ptr &rhs)
{
	tagged_node_ptr tmp = rhs;
	rhs = *this;
	*this = tmp;
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::tagged_node_ptr::is_leaf() const
{
	return off & unsigned(IS_LEAF);
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::tagged_node_ptr::get_leaf() const
{
	assert(is_leaf());
	return reinterpret_cast<radix_tree::leaf *>(get());
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node *
radix_tree<Key, Value, BytesView>::tagged_node_ptr::get_node() const
{
	assert(!is_leaf());
	return reinterpret_cast<radix_tree::node *>(get());
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator bool() const
	noexcept
{
	return (off & ~uint64_t(IS_LEAF)) != 0;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node *
	radix_tree<Key, Value, BytesView>::tagged_node_ptr::operator->() const
	noexcept
{
	return get_node();
}

template <typename Key, typename Value, typename BytesView>
uint64_t
radix_tree<Key, Value, BytesView>::tagged_node_ptr::get() const noexcept
{
	return reinterpret_cast<uint64_t>(this) + (off & ~uint64_t(IS_LEAF));
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::node::forward_iterator::forward_iterator(
	pointer child, const node *n)
    : child(child), n(n)
{
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator++()
{
	if (child == &n->embedded_entry)
		child = &n->child[0];
	else
		child++;

	return *this;
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::node::node(tagged_node_ptr parent,
					      byten_t byte, bitn_t bit)
    : parent(parent), byte(byte), bit(bit)
{
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator--()
{
	if (child == &n->child[0])
		child = &n->embedded_entry;
	else
		child--;

	return *this;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator++(int)
{
	forward_iterator tmp(child, n);
	operator++();
	return tmp;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator::reference
	radix_tree<Key, Value, BytesView>::node::forward_iterator::operator*()
		const
{
	return *child;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator::pointer
	radix_tree<Key, Value, BytesView>::node::forward_iterator::operator->()
		const
{
	return child;
}

template <typename Key, typename Value, typename BytesView>
typename radix_tree<Key, Value, BytesView>::node::forward_iterator::pointer
radix_tree<Key, Value, BytesView>::node::forward_iterator::get_node() const
{
	return n;
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator==(
	const forward_iterator &rhs) const
{
	return child == rhs.child && n == rhs.n;
}

template <typename Key, typename Value, typename BytesView>
bool
radix_tree<Key, Value, BytesView>::node::forward_iterator::operator!=(
	const forward_iterator &rhs) const
{
	return child != rhs.child || n != rhs.n;
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename std::enable_if<
	Direction ==
		radix_tree<Key, Value, BytesView>::node::direction::Forward,
	typename radix_tree<Key, Value,
			    BytesView>::node::forward_iterator>::type
radix_tree<Key, Value, BytesView>::node::begin() const
{
	return forward_iterator(&embedded_entry, this);
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename std::enable_if<
	Direction ==
		radix_tree<Key, Value, BytesView>::node::direction::Forward,
	typename radix_tree<Key, Value,
			    BytesView>::node::forward_iterator>::type
radix_tree<Key, Value, BytesView>::node::end() const
{
	return forward_iterator(&child[SLNODES], this);
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename std::enable_if<
	Direction ==
		radix_tree<Key, Value, BytesView>::node::direction::Reverse,
	typename radix_tree<Key, Value,
			    BytesView>::node::reverse_iterator>::type
radix_tree<Key, Value, BytesView>::node::begin() const
{
	return reverse_iterator(end<direction::Forward>());
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename std::enable_if<
	Direction ==
		radix_tree<Key, Value, BytesView>::node::direction::Reverse,
	typename radix_tree<Key, Value,
			    BytesView>::node::reverse_iterator>::type
radix_tree<Key, Value, BytesView>::node::end() const
{
	return reverse_iterator(begin<direction::Forward>());
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction, typename Ptr>
auto
radix_tree<Key, Value, BytesView>::node::find_child(const Ptr &n) const
	-> decltype(begin<Direction>())
{
	return std::find(begin<Direction>(), end<Direction>(), n);
}

template <typename Key, typename Value, typename BytesView>
template <bool Direction, typename Enable>
auto
radix_tree<Key, Value, BytesView>::node::make_iterator(
	const tagged_node_ptr *ptr) const -> decltype(begin<Direction>())
{
	return forward_iterator(ptr, this);
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
radix_tree<Key, Value, BytesView>::radix_tree_iterator<
	IsConst>::radix_tree_iterator(leaf_ptr leaf_, node_ptr root)
    : leaf_(leaf_), root(root)
{
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <bool C, typename Enable>
radix_tree<Key, Value, BytesView>::radix_tree_iterator<
	IsConst>::radix_tree_iterator(const radix_tree_iterator<false> &rhs)
    : leaf_(rhs.leaf_)
{
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst>::reference
	radix_tree<Key, Value,
		   BytesView>::radix_tree_iterator<IsConst>::operator*() const
{
	assert(leaf_);
	return *leaf_;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst>::pointer
	radix_tree<Key, Value,
		   BytesView>::radix_tree_iterator<IsConst>::operator->() const
{
	assert(leaf_);
	return leaf_;
}

/**
 * Handles assignment to the value. If there is enough capacity
 * old content is overwritten (with a help of undo log). Otherwise
 * a new leaf is allocated and the old one is freed.
 *
 * If reallocation happens, all other iterators to this element are invalidated.
 *
 * This function is useful when value_type is inline_string.
 */
template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <typename V, typename Enable>
void
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::assign_val(
	string_view rhs)
{
	auto pop = pool_base(pmemobj_pool_by_ptr(leaf_));

	if (rhs.size() <= leaf_->value().capacity()) {
		transaction::run(pop, [&] { leaf_->value() = rhs; });
	} else {
		tagged_node_ptr *slot;

		if (!leaf_->parent) {
			assert(root->get_leaf() == leaf_);
			slot = root;
		} else {
			slot = const_cast<tagged_node_ptr *>(
				&*leaf_->parent->find_child(leaf_));
		}

		auto old_leaf = leaf_;

		transaction::run(pop, [&] {
			*slot = leaf::make(old_leaf->parent, old_leaf->key(),
					   rhs);
			delete_persistent<typename radix_tree::leaf>(old_leaf);
		});

		leaf_ = slot->get_leaf();
	}
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <typename T, typename V, typename Enable>
void
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::assign_val(
	T &&rhs)
{
	leaf_->value() = std::forward<T>(rhs);
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst> &
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator++()
{
	assert(leaf_);

	/* leaf is root, there is no other leaf in the tree */
	if (!leaf_->parent)
		leaf_ = nullptr;
	else {
		auto it = leaf_->parent->template find_child<
			radix_tree::node::direction::Forward>(leaf_);

		leaf_ = const_cast<leaf_ptr>(
			next_leaf<radix_tree::node::direction::Forward>(
				it, leaf_->parent));
	}

	return *this;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
typename radix_tree<Key, Value,
		    BytesView>::template radix_tree_iterator<IsConst> &
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator--()
{
	if (!leaf_) {
		/* this == end() */
		leaf_ = const_cast<leaf_ptr>(
			radix_tree::find_leaf<
				radix_tree::node::direction::Reverse>(*root));
	} else {
		/* Iterator must be decrementable. */
		assert(leaf_->parent);

		auto it = leaf_->parent->template find_child<
			radix_tree::node::direction::Reverse>(leaf_);

		leaf_ = const_cast<leaf_ptr>(
			next_leaf<radix_tree::node::direction::Reverse>(
				it, leaf_->parent));
	}

	return *this;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <bool C>
bool
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator!=(
	const radix_tree_iterator<C> &rhs) const
{
	return leaf_ != rhs.leaf_;
}

template <typename Key, typename Value, typename BytesView>
template <bool IsConst>
template <bool C>
bool
radix_tree<Key, Value, BytesView>::radix_tree_iterator<IsConst>::operator==(
	const radix_tree_iterator<C> &rhs) const
{
	return !(*this != rhs);
}

/*
 * Returns next leaf (either with smaller or larger key, depending on
 * ChildIterator type). This function might need to traverse the
 * tree upwards.
 */
template <typename Key, typename Value, typename BytesView>
template <bool Direction, typename Iterator>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::next_leaf(Iterator node,
					     tagged_node_ptr parent)
{
	do {
		++node;
	} while (node != parent->template end<Direction>() && !(*node));

	/* No more children on this level, need to go up. */
	if (node == parent->template end<Direction>()) {
		auto p = parent->parent;
		if (!p) // parent == root
			return nullptr;

		auto p_it = p->template find_child<Direction>(parent);
		return next_leaf<Direction>(p_it, p);
	}

	return find_leaf<Direction>(*node);
}

/*
 * Returns smallest (or biggest, depending on ChildIterator) leaf
 * in a subtree.
 */
template <typename Key, typename Value, typename BytesView>
template <bool Direction>
typename radix_tree<Key, Value, BytesView>::leaf *
radix_tree<Key, Value, BytesView>::find_leaf(
	typename radix_tree<Key, Value, BytesView>::tagged_node_ptr n)
{
	assert(n);

	if (n.is_leaf())
		return n.get_leaf();

	for (auto it = n->template begin<Direction>();
	     it != n->template end<Direction>(); ++it) {
		if (*it)
			return find_leaf<Direction>(*it);
	}

	/* There must be at least one leaf at the bottom. */
	std::abort();
}

template <typename Key, typename Value, typename BytesView>
template <typename... Args>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make(tagged_node_ptr parent,
					      Args &&... args)
{
	auto ptr = make_internal(std::forward<Args>(args)...);
	ptr->parent = parent;

	return ptr;
}

template <typename Key, typename Value, typename BytesView>
Key &
radix_tree<Key, Value, BytesView>::leaf::key()
{
	return *reinterpret_cast<Key *>(this + 1);
}

template <typename Key, typename Value, typename BytesView>
Value &
radix_tree<Key, Value, BytesView>::leaf::value()
{
	auto key_dst = reinterpret_cast<char *>(this + 1);
	auto val_dst = reinterpret_cast<Value *>(key_dst +
						 real_size<Key>::value(key()));

	return *reinterpret_cast<Value *>(val_dst);
}

template <typename Key, typename Value, typename BytesView>
const Key &
radix_tree<Key, Value, BytesView>::leaf::key() const
{
	return *reinterpret_cast<const Key *>(this + 1);
}

template <typename Key, typename Value, typename BytesView>
const Value &
radix_tree<Key, Value, BytesView>::leaf::value() const
{
	auto key_dst = reinterpret_cast<const char *>(this + 1);
	auto val_dst = reinterpret_cast<const Value *>(
		key_dst + real_size<Key>::value(key()));

	return *reinterpret_cast<const Value *>(val_dst);
}

template <typename Key, typename Value, typename BytesView>
radix_tree<Key, Value, BytesView>::leaf::~leaf()
{
	detail::destroy<Key>(key());
	detail::destroy<Value>(value());
}

template <typename Key, typename Value, typename BytesView>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make_internal()
{
	auto t = std::make_tuple();
	return make_internal(std::piecewise_construct, t, t,
			     typename detail::make_index_sequence<>::type{},
			     typename detail::make_index_sequence<>::type{});
}

template <typename Key, typename Value, typename BytesView>
template <typename... Args1, typename... Args2>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make_internal(
	std::piecewise_construct_t pc, std::tuple<Args1...> first_args,
	std::tuple<Args2...> second_args)
{
	return make_internal(
		pc, first_args, second_args,
		typename detail::make_index_sequence<Args1...>::type{},
		typename detail::make_index_sequence<Args2...>::type{});
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make_internal(K &&k, V &&v)
{
	return make_internal(std::piecewise_construct,
			     std::forward_as_tuple(std::forward<K>(k)),
			     std::forward_as_tuple(std::forward<V>(v)));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make_internal(const K &k, const V &v)
{
	return make_internal(std::piecewise_construct, std::forward_as_tuple(k),
			     std::forward_as_tuple(v));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make_internal(std::pair<K, V> &&p)
{
	return make_internal(std::piecewise_construct,
			     std::forward_as_tuple(std::forward<K>(p.first)),
			     std::forward_as_tuple(std::forward<V>(p.second)));
}

template <typename Key, typename Value, typename BytesView>
template <typename K, typename V>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make_internal(const std::pair<K, V> &p)
{
	return make_internal(std::piecewise_construct,
			     std::forward_as_tuple(p.first),
			     std::forward_as_tuple(p.second));
}

template <typename Key, typename Value, typename BytesView>
template <typename... Args1, typename... Args2, size_t... I1, size_t... I2>
persistent_ptr<typename radix_tree<Key, Value, BytesView>::leaf>
radix_tree<Key, Value, BytesView>::leaf::make_internal(
	std::piecewise_construct_t, std::tuple<Args1...> &first_args,
	std::tuple<Args2...> &second_args, detail::index_sequence<I1...>,
	detail::index_sequence<I2...>)
{
	standard_alloc_policy<void> a;
	auto key_size = real_size<Key>::value(std::get<I1>(first_args)...);
	auto val_size = real_size<Value>::value(std::get<I2>(second_args)...);
	auto ptr = static_cast<persistent_ptr<leaf>>(
		a.allocate(sizeof(leaf) + key_size + val_size));

	auto key_dst = reinterpret_cast<Key *>(ptr.get() + 1);
	auto val_dst = reinterpret_cast<Value *>(
		reinterpret_cast<char *>(key_dst) + key_size);

	new (ptr.get()) leaf();
	new (key_dst) Key(std::forward<Args1>(std::get<I1>(first_args))...);
	new (val_dst) Value(std::forward<Args2>(std::get<I2>(second_args))...);

	return ptr;
}

/**
 * Private helper function. Checks if radix tree resides on pmem and throws an
 * exception if not.
 *
 * @throw pool_error if radix tree doesn't reside on pmem.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::check_pmem()
{
	if (nullptr == pmemobj_pool_by_ptr(this))
		throw pmem::pool_error("Invalid pool handle.");
}

/**
 * Private helper function. Checks if current transaction stage is equal to
 * TX_STAGE_WORK and throws an exception otherwise.
 *
 * @throw pmem::transaction_scope_error if current transaction stage is not
 * equal to TX_STAGE_WORK.
 */
template <typename Key, typename Value, typename BytesView>
void
radix_tree<Key, Value, BytesView>::check_tx_stage_work()
{
	if (pmemobj_tx_stage() != TX_STAGE_WORK)
		throw pmem::transaction_scope_error(
			"Function called out of transaction scope.");
}

/**
 * Non-member swap.
 */
template <typename Key, typename Value, typename BytesView>
void
swap(radix_tree<Key, Value, BytesView> &lhs,
     radix_tree<Key, Value, BytesView> &rhs)
{
	lhs.swap(rhs);
}

} /* namespace experimental */
} /* namespace obj */

namespace detail
{
template <>
struct bytes_view<obj::experimental::inline_string> {
	bytes_view(const obj::experimental::inline_string *s) : s(*s)
	{
	}

	char operator[](std::size_t p) const
	{
		return s[p];
	}

	size_t
	size() const
	{
		return s.size();
	}

	obj::string_view s;
};

template <typename T>
struct bytes_view<T,
		  typename std::enable_if<std::is_integral<T>::value &&
					  !std::is_signed<T>::value>::type> {
	bytes_view(const T *k) : k(k)
	{
#if __cpp_lib_endian
		static_assert(
			std::endian::native == std::endian::little,
			"Scalar type are not little endian on this platform!");
#elif !defined(NDEBUG)
		/* Assert little endian is used. */
		uint16_t word = (2 << 8) + 1;
		assert(((char *)&word)[0] == 1);
#endif
	}

	char operator[](std::ptrdiff_t p) const
	{
		return reinterpret_cast<const char *>(
			k)[static_cast<ptrdiff_t>(size()) - p - 1];
	}

	constexpr size_t
	size() const
	{
		return sizeof(T);
	}

	const T *k;
};
} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_RADIX_HPP */
