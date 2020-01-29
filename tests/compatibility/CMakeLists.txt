#
# Copyright 2020, Intel Corporation
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

include(../ctest_helpers.cmake)
include(ExternalProject)

set(COMPATIBILITY_TEST_HEADERS_LOCATION ${CMAKE_BINARY_DIR}/compatibility)

function(add_libpmemobj_cpp version)
    ExternalProject_Add(libpmemobj-cpp-${version}
        GIT_REPOSITORY https://github.com/pmem/libpmemobj-cpp
        CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${COMPATIBILITY_TEST_HEADERS_LOCATION}/${version}
                    -DBUILD_TESTS=OFF
                    -DBUILD_EXAMPLES=OFF
                    -DCMAKE_INSTALL_MESSAGE=NEVER
        GIT_TAG ${version}
    )
endfunction()

function(build_test_with_cpp_version version name)
    build_test(${name} ${ARGN})

    if (NOT("${version}" STREQUAL "master"))
        add_dependencies(${name} libpmemobj-cpp-${version})
    endif()

    target_include_directories(${name} BEFORE PUBLIC ${COMPATIBILITY_TEST_HEADERS_LOCATION}/${version}/include)
endfunction()

function(add_compatibility_test name executable1 executable2)
    add_test(NAME ${name}_0_none
        COMMAND ${CMAKE_COMMAND}
        ${GLOBAL_TEST_ARGS}
        -DTEST_NAME=${name}_0_none
        -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/${name}
        -DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/${executable}_0_none
        -DTEST_EXECUTABLE1=$<TARGET_FILE:${executable1}>
        -DTEST_EXECUTABLE2=$<TARGET_FILE:${executable2}>
        -DTRACER=none
        -DLONG_TESTS=${LONG_TESTS}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake)
endfunction()

add_libpmemobj_cpp(1.7)
add_libpmemobj_cpp(1.8.1)

build_test_with_cpp_version(1.7 version_1.7 version/version_1.7.cpp)
build_test_with_cpp_version(1.8.1 version_1.8.1 version/version_1.8.cpp)

build_test_with_cpp_version(1.8.1 concurrent_hash_map_feature_size_1.8.1 concurrent_hash_map_feature_size/concurrent_hash_map_feature_size.cpp)
build_test_with_cpp_version(master concurrent_hash_map_feature_size_master concurrent_hash_map_feature_size/concurrent_hash_map_feature_size.cpp)

add_compatibility_test(concurrent_hash_map_feature_size_1.8.1_master_compatibility concurrent_hash_map_feature_size_1.8.1 concurrent_hash_map_feature_size_master)
add_compatibility_test(concurrent_hash_map_feature_size_master_1.8.1_compatibility concurrent_hash_map_feature_size_master concurrent_hash_map_feature_size_1.8.1)
