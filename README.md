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
	- [Extra CMake flags - TODO](#extra-cmake-flags8)

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
cmake . -Bbuild -G "Visual Studio 15 2017" -A "Win64" -DCMAKE_TOOLCHAIN_FILE="c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"

msbuild build/ALL_BUILD.vcxproj /m
```
