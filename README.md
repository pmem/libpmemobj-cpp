libpmemobj-cpp
===============

[![Build Status](https://travis-ci.org/pmem/libpmemobj-cpp.svg?branch=master)](https://travis-ci.org/pmem/libpmemobj-cpp)
[![Build status](https://github.com/pmem/libpmemobj-cpp/workflows/CPP/badge.svg)](https://github.com/pmem/libpmemobj-cpp/actions)
[![libpmemobj-cpp version](https://img.shields.io/github/tag/pmem/libpmemobj-cpp.svg)](https://github.com/pmem/libpmemobj-cpp/releases/latest)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/15911/badge.svg)](https://scan.coverity.com/projects/pmem-libpmemobj-cpp)
[![Coverage Status](https://codecov.io/github/pmem/libpmemobj-cpp/coverage.svg?branch=master)](https://codecov.io/gh/pmem/libpmemobj-cpp/branch/master)

C++ bindings for libpmemobj (https://github.com/pmem/pmdk)
More information in include/libpmemobj++/README.md

# How to build #

## Requirements: ##
- cmake >= 3.3
- libpmemobj-dev(el) >= 1.8 (https://pmem.io/pmdk/)
- compiler with C++11 support:
	- GCC >= 4.8.1 (C++11 is supported in GCC since version 4.8.1, but it does not support expanding variadic template variables in lambda expressions, which is required to build persistent containers and is possible with GCC >= 4.9.0. If you want to build libpmemobj-cpp without testing containers, use flag TEST_XXX=OFF (separate flag for each container))
	- clang >= 3.3
- for testing and development:
	- valgrind-devel (at best with [pmemcheck support](https://github.com/pmem/valgrind))
	- clang format 9.0
	- perl

## On Linux ##

```sh
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```

#### When developing: ####
```sh
$ ...
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DDEVELOPER_MODE=1 -DCHECK_CPP_STYLE=1
$ ...
$ ctest --output-on-failure
```

#### To build packages ####
```sh
...
cmake .. -DCPACK_GENERATOR="$GEN" -DCMAKE_INSTALL_PREFIX=/usr
make package
```

$GEN is type of package generator and can be RPM or DEB

CMAKE_INSTALL_PREFIX must be set to a destination were packages will be installed

#### To use with Valgrind ####

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

## On Windows ##

#### Install libpmemobj via vcpkg ####
```sh
vcpkg install pmdk:x64-windows
vcpkg integrate install
```

```sh
...
cmake . -Bbuild -G "Visual Studio 14 2015 Win64"
        -DCMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake

msbuild build/ALL_BUILD.vcxproj
```
