# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

include(../ctest_helpers.cmake)
include(ExternalProject)

set(COMPATIBILITY_TEST_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/headers)

# Add external cmake projects with libpmemobj-cpp in selected {version}
# to build their headers (to include in compatbility tests).
function(add_libpmemobj_cpp version)
    ExternalProject_Add(libpmemobj-cpp-${version}
        GIT_REPOSITORY https://github.com/pmem/libpmemobj-cpp
        CMAKE_ARGS  -DCMAKE_INSTALL_PREFIX=${COMPATIBILITY_TEST_HEADERS}/${version}
                    -DBUILD_TESTS=OFF
                    -DBUILD_EXAMPLES=OFF
                    -DBUILD_DOC=OFF
                    -DBUILD_BENCHMARKS=OFF
                    -DCMAKE_INSTALL_MESSAGE=NEVER
        GIT_TAG ${version}
    )
endfunction()

# Build test ({name}) for selected {version} and include proper headers.
# Additional parameters ({ARGN}) are the test sources.
function(build_test_with_cpp_version version name)
    build_test(${name} ${ARGN})

    if (NOT("${version}" STREQUAL "master"))
        add_dependencies(${name} libpmemobj-cpp-${version})
    endif()

    target_include_directories(${name} BEFORE PUBLIC ${COMPATIBILITY_TEST_HEADERS}/${version}/include)
endfunction()

# Add compat. testcase run with 2 versions of the project (usually old ver. with master).
# Order is important, it checks backward and forward compatibility.
function(add_compatibility_test name executable1 executable2)
    add_test(NAME ${name}_0_none
        COMMAND ${CMAKE_COMMAND}
        ${GLOBAL_TEST_ARGS}
        -DTEST_NAME=${name}_0_none
        -DSRC_DIR=${CMAKE_CURRENT_SOURCE_DIR}/${name}
        -DBIN_DIR=${CMAKE_CURRENT_BINARY_DIR}/${name}_0_none
        -DTEST_EXECUTABLE1=$<TARGET_FILE:${executable1}>
        -DTEST_EXECUTABLE2=$<TARGET_FILE:${executable2}>
        -DTRACER=none
        -DLONG_TESTS=${LONG_TESTS}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run.cmake)
endfunction()

add_libpmemobj_cpp(1.7)
add_libpmemobj_cpp(1.8.1)
add_libpmemobj_cpp(1.9)
add_libpmemobj_cpp(1.10)
add_libpmemobj_cpp(1.11)

build_test_with_cpp_version(1.7 version_1.7 version/version_1.7.cpp)
build_test_with_cpp_version(1.8.1 version_1.8.1 version/version_1.8.cpp)
build_test_with_cpp_version(1.9 version_1.9 version/version_1.9.cpp)
build_test_with_cpp_version(1.10 version_1.10 version/version_1.10.cpp)
build_test_with_cpp_version(1.11 version_1.11 version/version_1.11.cpp)

build_test_with_cpp_version(1.8.1 concurrent_hash_map_feature_size_1.8.1 concurrent_hash_map_feature_size/concurrent_hash_map_feature_size.cpp)
build_test_with_cpp_version(1.9 concurrent_hash_map_feature_size_1.9 concurrent_hash_map_feature_size/concurrent_hash_map_feature_size.cpp)
build_test_with_cpp_version(1.10 concurrent_hash_map_feature_size_1.10 concurrent_hash_map_feature_size/concurrent_hash_map_feature_size.cpp)
build_test_with_cpp_version(1.11 concurrent_hash_map_feature_size_1.11 concurrent_hash_map_feature_size/concurrent_hash_map_feature_size.cpp)
build_test_with_cpp_version(master concurrent_hash_map_feature_size_master concurrent_hash_map_feature_size/concurrent_hash_map_feature_size.cpp)

add_compatibility_test(concurrent_hash_map_feature_size_1.8.1_master_compatibility concurrent_hash_map_feature_size_1.8.1 concurrent_hash_map_feature_size_master)
add_compatibility_test(concurrent_hash_map_feature_size_master_1.8.1_compatibility concurrent_hash_map_feature_size_master concurrent_hash_map_feature_size_1.8.1)

add_compatibility_test(concurrent_hash_map_feature_size_1.9_master_compatibility concurrent_hash_map_feature_size_1.9 concurrent_hash_map_feature_size_master)
add_compatibility_test(concurrent_hash_map_feature_size_master_1.9_compatibility concurrent_hash_map_feature_size_master concurrent_hash_map_feature_size_1.9)

add_compatibility_test(concurrent_hash_map_feature_size_1.10_master_compatibility concurrent_hash_map_feature_size_1.10 concurrent_hash_map_feature_size_master)
add_compatibility_test(concurrent_hash_map_feature_size_master_1.10_compatibility concurrent_hash_map_feature_size_master concurrent_hash_map_feature_size_1.10)

add_compatibility_test(concurrent_hash_map_feature_size_1.11_master_compatibility concurrent_hash_map_feature_size_1.11 concurrent_hash_map_feature_size_master)
add_compatibility_test(concurrent_hash_map_feature_size_master_1.11_compatibility concurrent_hash_map_feature_size_master concurrent_hash_map_feature_size_1.11)
