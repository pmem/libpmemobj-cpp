/*
 * Copyright 2020, Intel Corporation
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

/**
 * @file
 * Defragmentation class.
 */

#ifndef LIBPMEMOBJ_CPP_DEFRAG_HPP
#define LIBPMEMOBJ_CPP_DEFRAG_HPP

#include <type_traits>
#include <vector>

#include <libpmemobj++/detail/template_helpers.hpp>
#include <libpmemobj++/persistent_ptr_base.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj/atomic_base.h>
#include <libpmemobj/base.h>

namespace pmem
{

namespace detail
{
using for_each_ptr_function = void (*)(obj::persistent_ptr_base &ptr);

template <typename T>
using t_has_for_each_ptr = typename std::enable_if<
	std::is_same<decltype(std::declval<T>().for_each_ptr(
			     std::declval<for_each_ptr_function>())),
		     void>::value>::type;

template <typename T>
using t_is_defragmentable = supports<T, t_has_for_each_ptr>;
}

namespace obj
{
/**
 * Checks if provided T type is defragmentable.
 * This is included in public API, but there's no need to check it
 * before calling 'add()', since 'add' method has a specialization
 * for doing nothing in case of non-defragmentable object.
 *
 * @return bool true when object implements 'for_each_ptr' method.
 */
template <typename T>
static constexpr typename std::enable_if<detail::t_is_defragmentable<T>::value,
					 bool>::type
is_defragmentable() noexcept
{
	return true;
}

/**
 * Checks if provided T type is defragmentable.
 * Specialization for non-defragmentable objects.
 *
 * @return bool false when object does not implement 'for_each_ptr' method.
 */
template <typename T>
static constexpr typename std::enable_if<!detail::t_is_defragmentable<T>::value,
					 bool>::type
is_defragmentable() noexcept
{
	return false;
}

/**
 * Defrag class.
 *
 * This class implements methods used to store pointers from a pool.
 * When defragmentation is called/run, all objects previously stored
 * will be a subject of a defragmentation process.
 *
 * Important note: an instance of this class can collect pointers
 * only from one pmem::obj::pool instance.
 *
 * The typical usage example would be:
 * @snippet doc_snippets/defrag.cpp defrag_usage_example
 */
class defrag {
public:
	/**
	 * The only allowed ctor. Binds this object with the selected pool.
	 * It will limit pointers, while adding to the defragmentation queue,
	 * to only these from that pool. Otherwise during any 'add()' call
	 * the runtime_error will be thrown.
	 *
	 * @param[in] p a pool, which defrag will be working with/on.
	 */
	defrag(pool_base p)
	{
		this->pop = p;
	}

	/**
	 * Stores address of the referenced object to the defragmentation queue.
	 *
	 * It's to be called on objects (most likely containers), which are
	 * defragmentable (they implement 'for_each_ptr' method).
	 *
	 * @param[in] t object of type T to be added to the defragmentation
	 *	queue.
	 *
	 * @throw std::runtime_error when object t is not from the pool
	 *	passed in ctor.
	 */
	template <typename T>
	typename std::enable_if<is_defragmentable<T>(), void>::type
	add(T &t)
	{
		if (pmemobj_pool_by_ptr(&t) != pop.handle())
			throw std::runtime_error(
				"object is not from the chosen pool");

		t.for_each_ptr([&](persistent_ptr_base &ptr) {
			this->container.push_back(&ptr);
		});
	}

	/**
	 * Specialization for non-defragmentable types.
	 *
	 * It's to be called on (most likely trivial) types, which are NOT
	 * defragmentable - it means they don't implement 'for_each_ptr' method.
	 *
	 * Thanks to this specialization there's no need to check for each
	 * object, if it's defragmentable, before calling 'add()'.
	 *
	 * @param[in] t object of type T to be added to the defragmentation
	 *	queue.
	 *
	 * @throw std::runtime_error when object t is not from the pool
	 *	passed in ctor.
	 */
	template <typename T, typename = T>
	typename std::enable_if<!is_defragmentable<T>(), void>::type
	add(T &t)
	{
		if (pmemobj_pool_by_ptr(&t) != pop.handle())
			throw std::runtime_error(
				"object is not from the chosen pool");
	}

	/**
	 * Stores address of a persistent_ptr to the defragmentation queue.
	 *
	 * It's to be called for a 'persistent_ptr<T>' objects.
	 * If 'is_defragmentable<T>()' returns true, the underlying (T) object
	 * will also be added to the defragmentation queue.
	 *
	 * @param[in] ptr reference of the persistent_ptr to be added
	 *	to the defragmentation queue.
	 *
	 * @throw std::runtime_error when ptr does not point to an object
	 *	from the pool passed in ctor.
	 */
	template <typename T>
	void
	add(persistent_ptr<T> &ptr)
	{
		if (pmemobj_pool_by_oid(ptr.raw()) != pop.handle())
			throw std::runtime_error(
				"persistent_ptr does not point to an object from the chosen pool");

		this->container.push_back(&ptr);
		/* Calls 'add(T &)' passing the underlying object (T) */
		this->add<T>(*ptr);
	}

	/**
	 * Starts defragmentation with previously stored pointers.
	 *
	 * Note: It can be run within a transaction, but it will only work if
	 * the objects selected for the defragmentation won't be changed.
	 *
	 * @return result struct containing a number of relocated and total
	 *	processed objects.
	 *
	 * @throw rethrows pmem::defrag_error when a failure during
	 *	defragmentation occurs. Even if this error is thrown,
	 *	some of objects could have been relocated,
	 *	see in such case defrag_error.result for summary stats.
	 */
	pobj_defrag_result
	run()
	{
		pobj_defrag_result result = this->pop.defrag(
			this->container.data(), this->container.size());

		return result;
	}

private:
	std::vector<persistent_ptr_base *> container;
	pool_base pop;
};

} /* namespace obj */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_CPP_DEFRAG_HPP */
