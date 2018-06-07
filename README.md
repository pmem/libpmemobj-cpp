# libpmemobj-cpp
C++ bindings for libpmemobj (https://github.com/pmem/pmdk)
More informations in include/libpmemobj++/README.md

# How to build #

Requirements:
- cmake >= 3.3
- libpmemobj-dev(el) >= 1.4 (http://pmem.io/pmdk/)

```sh
$ mkdir build
$ cd build
$ cmake ..
$ make
$ make install
```

When developing:
```sh
$ ...
$ cmake .. -DCMAKE_BUILD_TYPE=Debug -DDEVELOPER_MODE=1
$ ...
$ ctest --output-on-failure
```

If you wish to run C++ standard library containers tests, you need to set the path to your custom versions of either gcc or
libc++. For gcc run cmake like this:
```sh
...
$ cmake .. -DUSE_CUSTOM_GCC=1 -DGCC_INCDIR=/path/to/includes -DGCC_LIBDIR=/path/to/lib
...
```

If you want to use a custom version of libc++ run:
```sh
...
$ cmake .. -DUSE_CUSTOM_LLVM=1 -DLIBCPP_INCDIR=/usr/local/libcxx/include/c++/v1 -DLIBCPP_LIBDIR=/usr/local/libcxx/lib
...
```

# Packaging
```sh
...
cmake .. -DCPACK_GENERATOR="$GEN" -DCMAKE_INSTALL_PREFIX=/usr
make package
```

$GEN is type of package generator and can be RPM or DEB

CMAKE_INSTALL_PREFIX must be set to a destination were packages will be installed
