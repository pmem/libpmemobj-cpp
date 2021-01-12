// SPDX-License-Identifier: BSD-3-Clause
/* Copyright 2019-2020, Intel Corporation */

#include "list_wrapper.hpp"
#include "unittest.hpp"

namespace nvobj = pmem::obj;

using vector_type = container_t<int>;
using vector_representation = container_representation_t<int>;

#if defined(VECTOR)
struct check_members_order {
	check_members_order() : vector()
	{
		vector.reserve(100);

		UT_ASSERTeq(representation.size, 0);
		UT_ASSERTeq(representation.capacity,
			    expected_capacity<size_t>(100));

		vector.resize(200);

		UT_ASSERTeq(representation.size, 200);

		vector[10] = 123456789;

		UT_ASSERTeq(representation.ptr[10], 123456789);
	}

	~check_members_order()
	{
		vector.~vector_type();
	}

	union {
		vector_type vector;
		vector_representation representation;
	};
};
#elif defined(SEGMENT_VECTOR_ARRAY_EXPSIZE) ||                                 \
	defined(SEGMENT_VECTOR_VECTOR_EXPSIZE)
struct check_members_order {
	check_members_order() : vector()
	{
		vector.reserve(100);

		UT_ASSERTeq(representation.segments_used, 7);
		UT_ASSERTeq(vector.capacity(), expected_capacity<size_t>(100));

		vector.resize(200);

		UT_ASSERTeq(representation.segments_used, 8);
		UT_ASSERTeq(vector.size(), 200);

		vector[10] = 123456789;

		size_t segment_idx =
			static_cast<size_t>(pmem::detail::Log2(10 | 1));
		size_t local_idx = 10 - (size_t(1) << segment_idx);
		UT_ASSERTeq(representation.ptr[segment_idx][local_idx],
			    123456789);
	}

	~check_members_order()
	{
		vector.~vector_type();
	}

	union {
		vector_type vector;
		vector_representation representation;
	};
};
#elif defined(SEGMENT_VECTOR_VECTOR_FIXEDSIZE)
struct check_members_order {
	check_members_order() : vector()
	{
		vector.reserve(100);

		UT_ASSERTeq(representation.segments_used, 1);
		UT_ASSERTeq(vector.capacity(), expected_capacity<size_t>(100));

		vector.resize(200);

		UT_ASSERTeq(representation.segments_used, 2);
		UT_ASSERTeq(vector.size(), 200);

		vector[10] = 123456789;

		UT_ASSERTeq(representation.ptr[0][10], 123456789);
	}

	~check_members_order()
	{
		vector.~vector_type();
	}

	union {
		vector_type vector;
		vector_representation representation;
	};
};
#endif

struct root {
	nvobj::persistent_ptr<check_members_order> v;
};

/* verify if members of vector are in proper order */
void
check_members_order_f(nvobj::pool<struct root> &pop)
{
	auto r = pop.root();

	try {
		nvobj::transaction::run(pop, [&] {
			r->v = nvobj::make_persistent<check_members_order>();
		});
	} catch (std::exception &e) {
		UT_FATALexc(e);
	}
}

static void
test(int argc, char *argv[])
{
	if (argc < 2) {
		UT_FATAL("usage: %s file-name", argv[0]);
	}

	auto path = argv[1];
	auto pop = nvobj::pool<root>::create(
		path, "VectorTest", PMEMOBJ_MIN_POOL * 2, S_IWUSR | S_IRUSR);

	static_assert(sizeof(container_t<int>) == expected_sizeof(), "");
	static_assert(sizeof(container_t<char>) == expected_sizeof(), "");
	static_assert(
		sizeof(container_t<container_t<int>>) == expected_sizeof(), "");

	static_assert(std::is_standard_layout<vector_type>::value, "");

	check_members_order_f(pop);

	pop.close();
}

int
main(int argc, char *argv[])
{
	return run_test([&] { test(argc, argv); });
}
