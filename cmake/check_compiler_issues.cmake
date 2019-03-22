#
# Copyright 2019, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if(NOT MSVC_VERSION)
	set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
	set(SAVED_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})

	# Check for issues with older gcc compilers which do not expand variadic template
	# variables in lambda expressions.
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

	# Check for issues with older gcc compilers if "inline" aggregate initialization
	# works for array class members https://gcc.gnu.org/bugzilla/show_bug.cgi?id=65815
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

	# Check for issues related to agregate initialization in new expression.
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
	set(CMAKE_REQUIRED_INCLUDES ${CMAKE_SOURCE_DIR}/include ${LIBPMEMOBJ_INCLUDE_DIRS})
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -Wno-error -c")
	CHECK_CXX_SOURCE_COMPILES(
		"#include <libpmemobj++/make_persistent_array.hpp>
		using namespace pmem::obj;
		int main() {
			delete_persistent<int[][3]>(make_persistent<int[][3]>(2), 2);
			return 0;
		}"
		NO_CLANG_TEMPLATE_BUG)

	# This is a workaround for older incompatible versions of libstdc++ and clang.
	# Please see https://llvm.org/bugs/show_bug.cgi?id=15517 for more info.
	set(CMAKE_REQUIRED_FLAGS "--std=c++11 -Wno-error -include future")
	CHECK_CXX_SOURCE_COMPILES(
		"int main() { return 0; }"
		NO_CHRONO_BUG)
else()
	set(NO_GCC_VARIADIC_TEMPLATE_BUG TRUE)
	set(NO_GCC_AGGREGATE_INITIALIZATION_BUG TRUE)
	set(NO_CLANG_BRACE_INITIALIZATION_NEWEXPR_BUG TRUE)
	set(NO_CLANG_TEMPLATE_BUG TRUE)
	set(NO_CHRONO_BUG TRUE)
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

set(CMAKE_REQUIRED_INCLUDES ${SAVED_CMAKE_REQUIRED_INCLUDES})
set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})
