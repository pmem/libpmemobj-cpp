# libpmemobj-cpp
===============

[![Build Status](https://travis-ci.org/pmem/libpmemobj-cpp.svg?branch=master)](https://travis-ci.org/pmem/libpmemobj-cpp)
[![Build status](https://github.com/pmem/libpmemobj-cpp/workflows/CPP/badge.svg)](https://github.com/pmem/libpmemobj-cpp/actions)
[![libpmemobj-cpp version](https://img.shields.io/github/tag/pmem/libpmemobj-cpp.svg)](https://github.com/pmem/libpmemobj-cpp/releases/latest)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/15911/badge.svg)](https://scan.coverity.com/projects/pmem-libpmemobj-cpp)
[![Coverage Status](https://codecov.io/github/pmem/libpmemobj-cpp/coverage.svg?branch=master)](https://codecov.io/gh/pmem/libpmemobj-cpp/branch/master)
[![Packaging status](https://repology.org/badge/tiny-repos/libpmemobj-cpp.svg)](https://repology.org/project/libpmemobj-cpp/versions)

**libpmemobj-cpp** is a C++ binding for **libpmemobj** (a library which is a part of [PMDK collection](https://github.com/pmem/pmdk)).
More implementation details can be found in [include/libpmemobj++/README.md](include/libpmemobj++/README.md).

Latest releases can be found on the ["releases" tab](https://github.com/pmem/libpmemobj-cpp/releases).
Up-to-date support/maintenance status of branches/releases is available on [pmem.io](https://pmem.io/libpmemobj-cpp).

# Table of contents
- [Overview](#libpmemobj-cpp)
- [Build instructions](#build-instructions)
	- [Requirements](#requirements)
		- [Additional requirements](#additional-requirements)
	- [Linux build](#linux-build)
	  - [Standard compilation](#standard-compilation)
	  - [Developer compilation](#developer-compilation)
	  - [Distribution package build](#distribution-package-build)
	- [Windows build](#windows-build)
	  - [Libpmemobj++ installation via vcpkg](#libpmemobj++-installation-via-vcpkg)
	  - [Install prerequisites via vcpkg](#install-prerequisites-via-vcpkg)
	  - [Compilation with Visual Studio 2015](#compilation-with-visual-studio-2015)
	  - [Compilation with Visual Studio 2017 or above](#compilation-with-visual-studio-2017-or-above)
	- [Extra CMake compilation flags](#extra-cmake-compilation-flags)
	  - [General flags](#general-flags)
	  - [Container related flags](#container-related-flags)

# Build instructions

## Requirements
- cmake >= 3.3
- libpmemobj-dev(el) >= 1.8 (https://pmem.io/pmdk/)
- compiler with C++11 support:
	- GCC >= 4.8.1 (C++11 is supported in GCC since version 4.8.1, but it does not support expanding variadic template variables in lambda expressions, which is required to build persistent containers and is possible with GCC >= 4.9.0. If you want to build libpmemobj-cpp without testing containers, use flag TEST_XXX=OFF (separate flag for each container)) TODO:update after adding flags section
	- clang >= 3.3
	- msbuild >= 14
- for testing and development:
	- valgrind-devel (at best with [pmemcheck support](https://github.com/pmem/valgrind))
	- clang format 9.0
	- perl
- for Windows compilation:
	- [vcpkg](https://github.com/microsoft/vcpkg#quick-start-windows)

### Additional requirements
**radix_tree**: on Windows, Visual Studio in version at least 2017 is needed. Testing radix_tree can be disabled via CMake options (use -DTEST_RADIX_TREE=OFF).

## Linux build
### Standard compilation
```sh
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```

### Developer compilation
```sh
$ ...
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DDEVELOPER_MODE=1 -DCHECK_CPP_STYLE=1
$ ...
$ ctest --output-on-failure
```

### Distribution package build
```sh
...
$ cmake .. -DCPACK_GENERATOR="$GEN" -DCMAKE_INSTALL_PREFIX=/usr
$ make package
```

$GEN is type of package generator and can be RPM or DEB

CMAKE_INSTALL_PREFIX must be set to a destination were packages will be installed

#### To use with Valgrind

In order to build your application with libpmemobj-cpp and
[pmemcheck](https://github.com/pmem/valgrind) / memcheck / helgrind / drd,
Valgrind instrumentation must be enabled during compilation by adding flags:
- LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED=1 for pmemcheck instrumentation,
- LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED=1 for memcheck instrumentation,
- LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED=1 for helgrind instrumentation,
- LIBPMEMOBJ_CPP_VG_DRD_ENABLED=1 for drd instrumentation, or
- LIBPMEMOBJ_CPP_VG_ENABLED=1 for all Valgrind instrumentations (including pmemcheck).

If there are no memcheck / helgrind / drd / pmemcheck headers installed on your
system, build will fail.

## Windows build
### Libpmemobj++ installation via vcpkg
```sh
.\vcpkg.exe install libpmemobj-cpp:x64-windows
```

### Install prerequisites via vcpkg
```sh
vcpkg install pmdk:x64-windows
vcpkg integrate install
```

### Compilation with Visual Studio 2015
```sh
cmake . -Bbuild -G "Visual Studio 14 2015 Win64"
        -DCMAKE_TOOLCHAIN_FILE="c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
        -DTEST_RADIX_TREE=OFF

msbuild build/ALL_BUILD.vcxproj /m
```
### Compilation with Visual Studio 2017 or above
```sh
cmake . -Bbuild -G "Visual Studio 15 2017" -A "x64" -DCMAKE_TOOLCHAIN_FILE="c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"

msbuild build/ALL_BUILD.vcxproj /m
```

## Extra CMake compilation flags

### General flags
| Flag name | Description | Default setting |
| --------- | ----------- | --------------- |
| BUILD_EXAMPLES | Enable examples compilation | ON |
| BUILD_TESTS | Enable tests compilation | ON |
| BUILD_DOC | Enable doxygen documentation generation | ON |
| BUILD_BENCHMARKS | Enable compilation of benchmarks | ON |
| COVERAGE | Run coverage test | OFF |
| DEVELOPER_MODE | Enable developer checks | OFF |
| CHECK_CPP_STYLE | Check code style of C++ sources | OFF |
| TRACE_TESTS | Enable more verbose test outputs | OFF |
| USE_ASAN | Enable AddressSanitizer (debugging) | OFF |
| USE_UBSAN | Enable UndefinedBehaviorSanitizer (debugging) | OFF |
| USE_CCACHE | Use ccache if it is available in the system | ON |
| TESTS_USE_FORCED_PMEM | Run tests with PMEM_IS_PMEM_FORCE=1 - it speeds up tests execution on emulated pmem | OFF |
| TESTS_USE_VALGRIND | Enable tests with valgrind (if found) | ON |
| TESTS_PMREORDER | Enable tests with pmreorder (if pmreorder found; it requires PMDK ver. >= 1.9) | OFF |
| TESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND | Enable concurrent_hash_map tests with drd and helgrind (can only be run on PMEM) | OFF |
| TESTS_CONCURRENT_GDB | Enable concurrent GDB tests - require 'set scheduler-locking on' support (OS dependent) | OFF |
| TESTS_LONG | Enable time consuming tests | OFF |
| TESTS_TBB | Enable tests which require TBB | OFF |
| TESTS_COMPATIBILITY | Enable compatibility tests (requires internet connection) | OFF |

### Container related flags
| Flag name | Description | Default setting |
| --------- | ----------- | --------------- |
| TEST_ARRAY | Enable testing of pmem::obj::array | ON |
| TEST_VECTOR | Enable testing of pmem::obj::vector" | ON |
| TEST_STRING | Enable testing of pmem::obj::string (depends on TEST_VECTOR) | ON |
| TEST_CONCURRENT_HASHMAP | Enable testing of pmem::obj::concurrent_hash_map (depends on TEST_STRING) | ON |
| TEST_SEGMENT_VECTOR_ARRAY_EXPSIZE | Enable testing of pmem::obj::segment_vector with array as segment_vector_type and exponential_size_policy | ON |
| TEST_SEGMENT_VECTOR_VECTOR_EXPSIZE | Enable testing of pmem::obj::segment_vector with vector as segment_vector_type and exponential_size_policy | ON |
| TEST_SEGMENT_VECTOR_VECTOR_FIXEDSIZE | Enable testing of pmem::obj::segment_vector with vector as segment_vector_type and fixed_size_policy | ON |
| TEST_ENUMERABLE_THREAD_SPECIFIC | Enable testing of pmem::obj::enumerable_thread_specific | ON |
| TEST_CONCURRENT_MAP | Enable testing of pmem::obj::experimental::concurrent_map (depends on TEST_STRING) | ON |
| TEST_SELF_RELATIVE_POINTER | Enable testing of pmem::obj::experimental::self_relative_ptr | ON |
| TEST_RADIX_TREE | Enable testing of pmem::obj::experimental::radix_tree | ON |