# libpmemobj-cpp

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

## Compatibility note
In libpmemobj 1.12 we introduced a new transaction handler type: [pmem::obj::flat_transaction](https://pmem.io/libpmemobj-cpp/master/doxygen/classpmem_1_1obj_1_1flat__transaction.html).
By defining LIBPMEMOBJ_CPP_USE_FLAT_TRANSACTION you can make pmem::obj::transaction to be an alias to pmem::obj::flat_transaction.
In 1.12 we have also changed the default behavior of containers' transactional methods. Now, in case of any failure within such method,
the outer transaction (if any) will not be immediately aborted. Instead, an exception will be thrown, which will lead to transaction abort
only if it's not caught before the outer tx scope ends. To change the behavior to the old one, you can set LIBPMEMOBJ_CPP_FLAT_TX_USE_FAILURE_RETURN macro to 0.
Be aware that the old behavior can lead to segfaults in some cases (see tx_nested_struct_example in this [file](examples/transaction/transaction.cpp)).

## Table of contents
1. [Pre-built packages](#pre-built-packages)
	- [Windows](#windows)
	- [Ubuntu/Debian](#ubuntudebian)
	- [Fedora/RHEL](#fedorarhel)
2. [Dependencies](#dependencies)
3. [Linux build](#linux-build)
	- [Standard compilation](#standard-compilation)
	- [Developer compilation](#developer-compilation)
	- [Distribution package build](#distribution-package-build)
	- [Compilation with Valgrind instrumentation](#compilation-with-valgrind-instrumentation)
4. [Windows build](#windows-build)
	- [Install prerequisites via vcpkg](#install-prerequisites-via-vcpkg)
	- [Compilation with Visual Studio 2015](#compilation-with-visual-studio-2015)
	- [Compilation with Visual Studio 2017 or above](#compilation-with-visual-studio-2017-or-above)
5. [Extra CMake compilation flags](#extra-cmake-compilation-flags)
6. [Contact us](#contact-us)

## Pre-built packages
The best way to install stable releases, tested on specific OS, is to use package manager.

### Windows
The recommended and the easiest way to install **libpmemobj-cpp** on Windows is to use Microsoft's vcpkg. Vcpkg is an open source tool and ecosystem created for library management.
For more information about vcpkg please see [vcpkg repository](https://github.com/microsoft/vcpkg#quick-start-windows).
```ps
.\vcpkg.exe install libpmemobj-cpp:x64-windows
```

### Ubuntu/Debian
For installation on Debian-related distros please execute following commands:
```
# apt install libpmemobj-cpp-dev
```

### Fedora/RHEL
To install **libpmemobj-cpp** on Fedora or RedHat execute:
```
# dnf install libpmemobj++-devel
```

## Dependencies
You will need the following packages for compilation:

- **cmake** >= 3.3
- **libpmemobj-dev(el)** >= 1.9 (https://pmem.io/pmdk/)
- compiler with C++11 support
	- **gcc** >= 4.8.1<sup> 1</sup>
	- **clang** >= 3.3
	- **msbuild** >= 14<sup> 2</sup>
- for testing and development:
	- **valgrind-devel** (at best with [pmemcheck support](https://github.com/pmem/valgrind))
	- **clang-format** 9.0
	- **perl**
- for Windows compilation:
	- [**vcpkg**](https://github.com/microsoft/vcpkg#quick-start-windows)

 ><sup>1</sup> C++11 is supported in GCC since version 4.8.1, but it does not support expanding variadic template variables in lambda expressions, which is required to build persistent containers and is possible with GCC >= 4.9.0. If you want to build libpmemobj-cpp without testing containers, use flag TEST_XXX=OFF (separate flag for each container).

 ><sup>2</sup> **radix_tree** is supported on Windows with MSBuild >=15 (Visual Studio at least 2017 is needed). Testing radix_tree can be disabled via CMake option (use -DTEST_RADIX_TREE=OFF).

## Linux build
### Standard compilation
```
$ mkdir build
$ cd build
$ cmake ..
$ make [-DCMAKE_INSTALL_PREFIX=<path_to_installation_dir>]
# make install
```

### Developer compilation
```sh
$ mkdir build
$ cd build
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DDEVELOPER_MODE=1 -DCHECK_CPP_STYLE=1
$ make
$ ctest --output-on-failure
```

### Distribution package build
```sh
$ mkdir build
$ cd build
$ cmake .. -DCPACK_GENERATOR="$GEN" -DCMAKE_INSTALL_PREFIX=/usr
$ make package
```

$GEN is type of package generator and can be RPM or DEB

CMAKE_INSTALL_PREFIX must be set to a destination were packages will be installed

### Compilation with Valgrind instrumentation

In order to build your application with libpmemobj-cpp and
[pmemcheck](https://github.com/pmem/valgrind) / memcheck / helgrind / drd,
Valgrind instrumentation must be enabled during compilation by adding flags:
- LIBPMEMOBJ_CPP_VG_PMEMCHECK_ENABLED=1 for pmemcheck instrumentation
- LIBPMEMOBJ_CPP_VG_MEMCHECK_ENABLED=1 for memcheck instrumentation
- LIBPMEMOBJ_CPP_VG_HELGRIND_ENABLED=1 for helgrind instrumentation
- LIBPMEMOBJ_CPP_VG_DRD_ENABLED=1 for drd instrumentation, or
- LIBPMEMOBJ_CPP_VG_ENABLED=1 for all Valgrind instrumentations (including pmemcheck).

If there are no memcheck / helgrind / drd / pmemcheck headers installed on your
system, build will fail.

## Windows build

### Install prerequisites via vcpkg
```ps
vcpkg install pmdk:x64-windows
vcpkg integrate install
```

### Compilation with Visual Studio 2015
```ps
cmake . -Bbuild -G "Visual Studio 14 2015 Win64"
        -DCMAKE_TOOLCHAIN_FILE="c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"
        -DTEST_RADIX_TREE=OFF

msbuild build/ALL_BUILD.vcxproj /m
```
### Compilation with Visual Studio 2017 or above
```ps
cmake . -Bbuild -G "Visual Studio 15 2017" -A "x64"
		-DCMAKE_TOOLCHAIN_FILE="c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake"

msbuild build/ALL_BUILD.vcxproj /m
```

## Extra CMake compilation flags
For custom build you can use CMake flags to change build type, change C++ standard, enable/disable components or features for testing purposes.
To list all CMake flags use the following:
```sh
$ mkdir build
$ cd build
$ cmake ..
$ cmake -LH
```
or just use graphical CMake frontend like **cmake-qt-gui** or **cmake-curses-gui**.

## Contact us
For more information on this library, contact Igor Chorążewicz (igor.chorazewicz@intel.com),
Piotr Balcer (piotr.balcer@intel.com) or post on our **#pmem** Slack channel using
[this invite link](https://join.slack.com/t/pmem-io/shared_invite/enQtNzU4MzQ2Mzk3MDQwLWQ1YThmODVmMGFkZWI0YTdhODg4ODVhODdhYjg3NmE4N2ViZGI5NTRmZTBiNDYyOGJjYTIyNmZjYzQxODcwNDg) or [Google group](https://groups.google.com/group/pmem).
