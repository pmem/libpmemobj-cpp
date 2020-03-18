---
title: C++ bindings
layout: main
---

#### The C++ bindings to libpmemobj

The **C++ bindings** provide a less error prone version of libpmemobj
through the implementation of a pmem-resident property, persistent pointers,
scoped and closure transactions, locking primitives and many others.

Doxygen documentation is available:

* for current [master](./master/doxygen/index.html)
* for [v1.9](./v1.9/doxygen/index.html)
* for [v1.8](./v1.8/doxygen/index.html)
* for [v1.7](./v1.7/doxygen/index.html)
* for [v1.6](./v1.6/doxygen/index.html)
* for [v1.5](./v1.5/doxygen/index.html)

#### Releases' support status

Currently last 3 branches/releases are fully supported. Latest releases can be
seen on the ["releases" tab on the GitHub page](https://github.com/pmem/libpmemobj-cpp/releases).

| Version branch | First release date | Last patch release | Maintenance status |
| -------------- | ------------------ | ------------------ | ------------------ |
| stable-1.9 | Jan 31, 2020 | N/A | Full |
| stable-1.8 | Oct 3, 2019 | 1.8.1 (Jan 24, 2020) | Full |
| stable-1.7 | Jun 26, 2019 | N/A | Full |
| stable-1.6 | Mar 15, 2019 | N/A | Limited |
| stable-1.5 | Oct 26, 2018 | 1.5.1 (Feb 19, 2019) | Limited |

Possible statuses:
1. Full maintenance:
	* All/most of bugs fixed (if possible),
	* Patch releases issued based on a number of fixes and their severity,
	* Full validation efforts for patch releases, including *long tests* execution,
	* At least one release at the end of the maintenance period,
	* Full support for at least a year since the initial release.
2. Limited scope:
	* Only critical bugs (security, data integrity, etc.) will be backported,
	* Patch versions will be released when needed (based on severity of found issues),
	* Validation of patch releases will include at least public CI (Travis, AppVeyor, GitHub Actions); *long tests* may not be executed,
	* Branch will remain in "limited maintenance" status based on the original release availability in popular distros,
3. EOL:
	* No support,
	* No bug fixes,
	* No official releases.

#### Blog entries

The following series of blog articles provides a tutorial introduction
to the **C++ bindings**:

* [Part 0 - Introduction](https://pmem.io/2016/01/12/cpp-01.html)
* [Part 1 - Pmem Resident Property](https://pmem.io/2016/01/12/cpp-02.html)
* [Part 2 - Persistent Smart Pointer](https://pmem.io/2016/01/12/cpp-03.html)
* [Part 3 - Persistent Queue Example](https://pmem.io/2016/01/12/cpp-04.html)
* [Part 4 - Pool Handle Wrapper](https://pmem.io/2016/05/10/cpp-05.html)
* [Part 5 - make_persistent](https://pmem.io/2016/05/19/cpp-06.html)
* [Part 6 - Transactions](https://pmem.io/2016/05/25/cpp-07.html)
* [Part 7 - Synchronization Primitives](https://pmem.io/2016/05/31/cpp-08.html)
* [Part 8 - Converting Existing Applications](https://pmem.io/2016/06/02/cpp-ctree-conversion.html)

There are also another blog posts regarding **C++ bindings**:
* [Modeling strings with libpmemobj C++ bindings](https://pmem.io/2017/01/23/cpp-strings.html)
* [Using Standard Library Containers with Persistent Memory](https://pmem.io/2017/07/10/cpp-containers.html)
* [C++ persistent containers - array](https://pmem.io/2018/11/02/cpp-array.html)
* [C++ persistent containers](https://pmem.io/2018/11/20/cpp-persistent-containers.html)
* [C++ persistent containers - vector](https://pmem.io/2019/02/20/cpp-vector.html)
* [C++ standard limitations and Persistent Memory](https://pmem.io/2019/10/04/cpp-limitations.html)
