# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

# This helper cmake file includes various tests for known compilers issues.

# Original CMake flags and includes are saved to be restored at the end of the file.
# This way project's original settings are not modified by the process of these checks.
set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
set(SAVED_CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
set(SAVED_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})

set(CMAKE_CXX_FLAGS "")

if(NOT MSVC_VERSION)

	# Even if we are ensuring that we use CMAKE_CXX_STANDARD >= 14, check if
	# shared_mutex header file is available for the current compiler version
	# because CXX_STANDARD is being set to 14 for --c++1y parameter
	if(CXX_STANDARD GREATER 11)
		set(CMAKE_REQUIRED_FLAGS "--std=c++${CMAKE_CXX_STANDARD} -c")
		CHECK_CXX_SOURCE_COMPILES(
			"#include <shared_mutex>
			int main() {}"
			NO_SHARED_MUTEX_BUG)
	else()
		set(NO_SHARED_MUTEX_BUG TRUE)
	endif()

	# Check for issues with older gcc compilers which do not expand variadic
	# template variables in lambda expressions. This functionality is being
	# used in libpmemobj-cpp containers, lack of its support resuls with
	# FATAL_ERROR unless you disable containers tests by setting CMake flag
	# TEST_XXX=OFF (separate flag for each container).
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -Wno-error -c")
	CHECK_CXX_SOURCE_COMPILES(
		"void print() {}
		template<typename...Args, typename T>
		void print(const T&, const Args &...arg) {
			auto f = [&]{ print(arg...);};
		}
		int main() {
			print(1, 2, 3);
			return 0;
		}"
		NO_GCC_VARIADIC_TEMPLATE_BUG)

	if(NOT NO_GCC_VARIADIC_TEMPLATE_BUG)
		if(TEST_ARRAY OR TEST_VECTOR OR TEST_STRING OR TEST_CONCURRENT_HASHMAP OR TEST_SEGMENT_VECTOR_ARRAY_EXPSIZE OR TEST_SEGMENT_VECTOR_VECTOR_EXPSIZE OR TEST_SEGMENT_VECTOR_VECTOR_FIXEDSIZE OR TEST_ENUMERABLE_THREAD_SPECIFIC)
			message(FATAL_ERROR
				"Compiler does not support expanding variadic template variables in lambda expressions. "
				"For more information about compiler requirements, check README.md.")
		elseif()
			message(WARNING
				"Compiler does not support expanding variadic template variables in lambda expressions. "
				"Some tests will be skipped and some functionalities won't be installed. "
				"For more information about compiler requirements, check README.md.")
		endif()
	endif()

	# Check for issues with older gcc compilers if "inline" aggregate initialization
	# works for array class members https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65815
	set(CMAKE_REQUIRED_FLAGS "")
	CHECK_CXX_SOURCE_COMPILES(
		"struct array {
			int data[2];
		};
		struct X {
			array a = { 1, 2 };
		};
		int main() {
			return 0;
		}"
		NO_GCC_AGGREGATE_INITIALIZATION_BUG)

	# Check for issues related to aggregate initialization in new expression.
	# Following code will fail for LLVM compiler https://bugs.llvm.org/show_bug.cgi?id=39988
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -Wno-error")
	CHECK_CXX_SOURCE_COMPILES(
		"template<typename T>
		struct A {
			 A() {};
			~A() {};
		};
		struct B {
			A<int> a;
			A<int> b;
		};
		int main() {
			new B{};
			return 0;
		}"
		NO_CLANG_BRACE_INITIALIZATION_NEWEXPR_BUG)

	# Check for issues with older clang compilers which assert on delete persistent<[][]>.
	set(CMAKE_REQUIRED_INCLUDES ${LIBPMEMOBJCPP_ROOT_DIR}/include ${LIBPMEMOBJ_INCLUDE_DIRS})
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -Wno-error -c")
	CHECK_CXX_SOURCE_COMPILES(
		"#include <libpmemobj++/make_persistent_array.hpp>
		using namespace pmem::obj;
		int main() {
			delete_persistent<int[][3]>(make_persistent<int[][3]>(2), 2);
			return 0;
		}"
		NO_CLANG_TEMPLATE_BUG)
	set(CMAKE_REQUIRED_INCLUDES "") # clean includes after check

	# This is a workaround for older incompatible versions of libstdc++ and clang.
	# Please see https://llvm.org/bugs/show_bug.cgi?id=15517 for more info.
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -Wno-error -include future")
	CHECK_CXX_SOURCE_COMPILES(
		"int main() { return 0; }"
		NO_CHRONO_BUG)
else()
	set(NO_SHARED_MUTEX_BUG TRUE)
	set(NO_GCC_VARIADIC_TEMPLATE_BUG TRUE)
	set(NO_GCC_AGGREGATE_INITIALIZATION_BUG TRUE)
	set(NO_CLANG_BRACE_INITIALIZATION_NEWEXPR_BUG TRUE)
	set(NO_CLANG_TEMPLATE_BUG TRUE)
	set(NO_CHRONO_BUG TRUE)
endif()

if(CXX_STANDARD LESS 14 OR NOT NO_SHARED_MUTEX_BUG)
	message(WARNING "volatile_state not supported (required C++14 compliant compiler)")
	set(VOLATILE_STATE_PRESENT OFF)
else()
	set(VOLATILE_STATE_PRESENT ON)
endif()

set(CMAKE_REQUIRED_FLAGS "--std=c++${CMAKE_CXX_STANDARD} -c")
CHECK_CXX_SOURCE_COMPILES(
	"#include <cstddef>
	int main() {
	    std::max_align_t var;
	    return 0;
	}"
	MAX_ALIGN_TYPE_EXISTS)

set(CMAKE_REQUIRED_FLAGS "--std=c++${CMAKE_CXX_STANDARD} -c")
CHECK_CXX_SOURCE_COMPILES(
	"#include <type_traits>
	int main() {
	#if !__cpp_lib_is_aggregate
		static_assert(false, \"\");
	#endif
	}"
	AGGREGATE_INITIALIZATION_AVAILABLE
)

# Restore original, project's settings
set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})
set(CMAKE_CXX_FLAGS ${SAVED_CMAKE_CXX_FLAGS})
set(CMAKE_REQUIRED_INCLUDES ${SAVED_CMAKE_REQUIRED_INCLUDES})
