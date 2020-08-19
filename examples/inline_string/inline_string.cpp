// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2020, Intel Corporation */

/**
 * inline_string.cpp -- example which shows how to use
 * pmem::obj::experimental::inline_string.
 */

#include <libpmemobj++/allocator.hpp>
#include <libpmemobj++/experimental/inline_string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>

#include <libpmemobj.h>

#include <iostream>

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
}

//! [inline_string_example]
struct Object {
	Object(int x, const char *s) : x(x), s(s)
	{
	}

	int x;

	/* Using inline_string instead of pmem::obj::string reduces number of
	 * allocations and dereferences which cost much more than on DRAM.
	 */
	pmem::obj::experimental::inline_string s;
};

struct root {
	pmem::obj::persistent_ptr<Object> o;
};

void
create_and_print_object(pmem::obj::pool<root> pop)
{
	auto r = pop.root();

	pmem::obj::transaction::run(pop, [&] {
		/* String was already allocated in a previous app run. */
		if (r->o)
			return;

		auto value = "example";

		/* There must be space for the Object itself and "example"
		 * string. */
		auto req_capacity =
			sizeof(Object) + strlen(value) + sizeof('\0');

		pmem::obj::allocator<void> a;
		r->o = static_cast<pmem::obj::persistent_ptr<Object>>(
			a.allocate(req_capacity));
		new (r->o.get()) Object(1, value);
	});

	std::cout << r->o->s.data() << std::endl;
}

void
assign_and_print_object(pmem::obj::pool<root> pop)
{
	auto r = pop.root();

	auto new_value = "some new, longer value";

	if (r->o->s.capacity() >= strlen(new_value)) {
		/* If there is enough capacity, we can assign the new value. */
		r->o->s.assign(new_value);
	} else {
		/* Otherwise we have to reallocate the whole object. */
		pmem::obj::transaction::run(pop, [&] {
			pmem::obj::allocator<void> a;
			auto ptr =
				static_cast<pmem::obj::persistent_ptr<Object>>(
					a.allocate(sizeof(Object) +
						   strlen(new_value) + 1));
			new (ptr.get()) Object(r->o->x, new_value);

			pmem::obj::delete_persistent<Object>(r->o);
			r->o = ptr;
		});
	}

	std::cout << r->o->s.data() << std::endl;
}
//! [inline_string_example]

int
main(int argc, char *argv[])
{
	if (argc < 2) {
		show_usage(argv);
		return 1;
	}

	const char *path = argv[1];

	pmem::obj::pool<root> pop;

	try {
		pop = pmem::obj::pool<root>::open(path, "inline_string");

		create_and_print_object(pop);
		assign_and_print_object(pop);
	} catch (pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr
			<< "To create pool run: pmempool create obj --layout=inline_string -s 100M path_to_pool"
			<< std::endl;
	} catch (std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	try {
		pop.close();
	} catch (const std::logic_error &e) {
		std::cerr << e.what() << std::endl;
	}
	return 0;
}
