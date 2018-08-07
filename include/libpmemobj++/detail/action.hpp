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

/**
 * @file
 * Wrappers for libpmemobj experimental action API.
 */

#ifndef LIBPMEMOBJ_ACTION_HPP
#define LIBPMEMOBJ_ACTION_HPP

#include "libpmemobj++/detail/pexceptions.hpp"
#include "libpmemobj++/persistent_ptr.hpp"
#include "libpmemobj/action.h"
#include <errno.h>
#include <vector>

namespace pmem
{

namespace detail
{

/**
 * action class
 *
 * This class is the pobj_action array handler. It provides basic primitives
 * for operations realted to reserve/publish API
 */
class action {
public:
	using iterator = std::vector<struct pobj_action>::iterator;

	/**
	 * Default constructor.
	 */
	action() noexcept : actv()
	{
	}

	/**
	 * Explicit constructor.
	 *
	 * Create action object based on given C-style pobj_action array.
	 */
	explicit action(struct pobj_action *cact, std::size_t actcnt)
	{
		for (struct pobj_action *it = cact; it != &cact[actcnt]; ++it)
			this->actv.push_back(*it);
	}

	/**
	 * Defaulted copy constructor.
	 */
	action(const action &) = default;

	/**
	 * Defaulted move constructor.
	 */
	action(action &&) noexcept = default;

	/**
	 * Defaulted copy assignment operator.
	 */
	action &operator=(const action &) = default;

	/**
	 * Defaulted move assignment operator.
	 */
	action &operator=(action &&) noexcept = default;

	/**
	 * Checks if there are actions stored in object.
	 */
	bool
	empty(void) const noexcept
	{
		return this->actv.empty();
	}

	/**
	 * Rreturns iterator to begin of actv.
	 */
	iterator
	begin(void) noexcept
	{
		return this->actv.begin();
	}

	/**
	 * Rreturns iterator to end of actv.
	 */
	iterator
	end(void) noexcept
	{
		return this->actv.end();
	}

	/**
	 * Returns number of actions stored in object.
	 */
	std::size_t
	size(void) const noexcept
	{
		return this->actv.size();
	}

	/**
	 * Performs a transient reservation of an object. The object returned
	 * by this function can be freely modified without worrying about
	 * fail-safe atomicity until the object has been published. Any
	 * modifications of the object must be manually persisted.
	 *
	 * @return persistent_ptr to reserved object
	 *
	 * @throw std::invalid_argument if unnkonw flag was given,
	 * std::runtime_error if reservation of an object failed.
	 */
	template <typename T>
	obj::persistent_ptr<T>
	reserve(obj::pool_base &pop, std::size_t size = sizeof(T),
		uint64_t flags = 0)
	{
		if (flags && (flags & ~POBJ_ACTION_XRESERVE_VALID_FLAGS))
			throw std::invalid_argument("Invalid flags argument.");

		obj::persistent_ptr<T> ret;
		struct pobj_action tmp;
		ret = pmemobj_xreserve(pop.get_handle(), &tmp, size,
				       detail::type_num<T>(), flags);
		if (ret == nullptr) {
			cancel(pop);
			throw std::runtime_error("Reservation failed.");
		}
		this->actv.push_back(tmp);

		return ret;
	}

	/**
	 * Prepares an action that, once published, will modify the memory
	 * location pointed to by ptr to value.
	 */
	void
	set_value(obj::pool_base &pop, uint64_t *ptr, uint64_t value)
	{
		struct pobj_action tmp;
		pmemobj_set_value(pop.get_handle(), &tmp, ptr, value);
		this->actv.push_back(tmp);
	}

	/**
	 * Creates a deferred free action, meaning that the provided object
	 * will be freed when the action is published.
	 */
	template <typename T>
	void
	defer_free(obj::pool_base &pop, obj::persistent_ptr<T> ptr)
	{
		struct pobj_action tmp;
		pmemobj_defer_free(pop.get_handle(), ptr.raw(), &tmp);
		this->actv.push_back(tmp);
	}

	/**
	 * Publishes all of actions in actv. The publication is fail-safe
	 * atomic. Once done, the persistent state will reflect the changes
	 * contained in the actions.
	 */
	void
	publish(obj::pool_base &pop)
	{
		publish(pop, this->actv.begin(), this->actv.end());
	}

	/**
	 * Publishes the provided set of actions. The publication is fail-safe
	 * atomic. Once done, the persistent state will reflect the changes
	 * contained in the actions.
	 */
	void
	publish(obj::pool_base &pop, iterator first, iterator last)
	{
		if (pmemobj_publish(pop.get_handle(), &*first,
				    static_cast<std::size_t>(
					    std::distance(first, last))) != 0)
			throw std::system_error(errno, std::generic_category());
	}

	/**
	 * Releases all resources held by actv and invalidates them.
	 */
	void
	cancel(obj::pool_base &pop)
	{
		cancel(pop, this->actv.begin(), this->actv.end());
	}

	/**
	 * Releases resources held by the provided set of actions and
	 * invalidates them. Ereases invalideted action elements from actv.
	 */
	void
	cancel(obj::pool_base &pop, iterator first, iterator last)
	{
		pmemobj_cancel(
			pop.get_handle(), &*first,
			static_cast<std::size_t>(std::distance(first, last)));

		erase(first, last);
	}

	/**
	 * Removes from the vector either a single element.
	 */
	iterator
	erase(iterator position)
	{
		return this->actv.erase(position);
	}

	/**
	 * Removes from the vector either a range of elements.
	 */
	iterator
	erase(iterator first, iterator last)
	{
		return this->actv.erase(first, last);
	}

private:
	/* Vector container to store pobj_action structures */
	std::vector<struct pobj_action> actv;
};

} /* namespace detail */

} /* namespace pmem */

#endif /* LIBPMEMOBJ_ACTION_HPP */
