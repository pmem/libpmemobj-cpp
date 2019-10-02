libpmemobj-cpp
===============

[![Build Status](https://travis-ci.org/pmem/libpmemobj-cpp.svg?branch=master)](https://travis-ci.org/pmem/libpmemobj-cpp)
[![Build status](https://ci.appveyor.com/api/projects/status/github/pmem/libpmemobj-cpp?branch/master?svg=true&pr=false)](https://ci.appveyor.com/project/pmem/libpmemobj-cpp/branch/master)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/15911/badge.svg)](https://scan.coverity.com/projects/pmem-libpmemobj-cpp)
[![Coverage Status](https://codecov.io/github/pmem/libpmemobj-cpp/coverage.svg?branch=master)](https://codecov.io/gh/pmem/libpmemobj-cpp/branch/master)

C++ bindings for libpmemobj (https://github.com/pmem/pmdk)
More information in include/libpmemobj++/README.md

# How to build #

## Requirements: ##
- cmake >= 3.3
- libpmemobj-dev(el) >= 1.7 (http://pmem.io/pmdk/)
- compiler with C++11 support

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
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DDEVELOPER_MODE=1
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
