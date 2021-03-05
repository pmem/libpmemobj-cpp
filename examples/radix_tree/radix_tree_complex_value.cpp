// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2021, Intel Corporation */

/**
 * radix_tree_complex_value.cpp -- example which shows how to use
 * pmem::obj::experimental::radix_tree with a custom value type.
 *
 * It maps names of some cities to theirs description.
 */

#include <libpmemobj++/experimental/radix_tree.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/pool.hpp>

#include <iostream>

struct date {
	pmem::obj::p<uint64_t> year;
	pmem::obj::p<uint64_t> month;
	pmem::obj::p<uint64_t> day;
};

/* Pmem-resident data type. */
struct city_info {
	city_info(uint64_t population, uint64_t area_in_sqr_km,
		  std::string country,
		  date upd_date = date{2021, 1, 1}) /* std::string
		     is passed here since pmem::obj::string
		     cannot be used on dram */
	    : population(population),
	      area_in_sqr_km(area_in_sqr_km),
	      country(country),
	      latest_update_date(upd_date)
	{
	}

	pmem::obj::p<uint64_t> population;
	pmem::obj::p<uint64_t> area_in_sqr_km;
	pmem::obj::string country;

	/* If some fields are often updated at the same time, it might
	 * be better to put them inside a structure and wrap the whole structure
	 * in pmem::obj::p<>. By doing this, number of snapshots can be reduced
	 * (which means better performance).
	 */
	pmem::obj::p<date> latest_update_date;
};

using kv_type = pmem::obj::experimental::radix_tree<
	pmem::obj::experimental::inline_string, city_info>;

struct root {
	pmem::obj::persistent_ptr<kv_type> kv;
};

void
show_usage(char *argv[])
{
	std::cerr << "usage: " << argv[0] << " file-name" << std::endl;
}

void
insert_elements_kv(pmem::obj::pool<root> pop)
{
	auto r = pop.root();

	/* Insert 3 kv pairs transactionally.
	 * The transaction guarantees that either all elements will be inserted
	 * or none of them (even in case of failure). */
	pmem::obj::transaction::run(pop, [&] {
		/* Ok, construct city_info directly on pmem */
		r->kv->try_emplace("Gdansk", 470907, 262, "Poland");
		r->kv->try_emplace("Warsaw", 1793579, 517, "Poland");
		r->kv->try_emplace("Krakow", 779115, 326, "Poland");

		/* WRONG: cannot create city_info on stack (DRAM) since
		 * pmem::obj::string can only be placed on pmem:
		 *
		 * city_info cs(470907, 262, "Poland");
		 * r->kv->try_emplace("Ponzań", cs);
		 *
		 * r->kv->try_emplace("Poznań", city_info(470907, 262,
		 * "Poland"));
		 */
	});

	kv_type::iterator it = r->kv->find("Gdansk");
	assert(it != r->kv->end());

	/* Update "Gdansk" information in a transaction.
	 * The transaction guarantees that either both population and area will
	 * be updated or none of them (even in case of failure).
	 *
	 * This code will make two snaphots.
	 */
	pmem::obj::transaction::run(pop, [&] {
		it->value().population += 10000;
		it->value().area_in_sqr_km += 10;
	});

	/* Update "Gdańsk" latest_update_date field.
	 *
	 * This code will result in only one snaphot.
	 */
	pmem::obj::transaction::run(pop, [&] {
		it->value().latest_update_date.get_rw().year = 2021;
		it->value().latest_update_date.get_rw().month = 3;
		it->value().latest_update_date.get_rw().day = 5;

		/* OR */
		it->value().latest_update_date = date{2021, 3, 5};
	});

	it = r->kv->find("Warsaw");
	assert(it != r->kv->end());

	/* Update a single value taking advantage of 8 byte atomicity:
	 * https://pmem.io/2015/06/13/accessing-pmem.html
	 */
	it->value().population += 20000;
	pop.persist(it->value().population);

	/* The following code may lead to inconsistent state (after restart,
	 * it's possible that only population is updated):
	 *
	 * it->value().population += 20000;
	 * pop.persist(it->value().population);
	 * it->value().area_in_sqr_km += 20;
	 * pop.persist(it->value().population);
	 */
}

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
		pop = pmem::obj::pool<root>::open(path, "radix");
		auto r = pop.root();

		if (r->kv == nullptr) {
			pmem::obj::transaction::run(pop, [&] {
				r->kv = pmem::obj::make_persistent<kv_type>();
			});
		}

		insert_elements_kv(pop);

	} catch (pmem::pool_error &e) {
		std::cerr << e.what() << std::endl;
		std::cerr
			<< "To create pool run: pmempool create obj --layout=radix -s 100M path_to_pool"
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
