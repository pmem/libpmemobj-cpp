libpmemobj-cpp	{#mainpage}
===========================

This is the C++ API for libpmemobj extended with persistent containers.

During the development of libpmemobj, many difficulties were encountered and
compromises were made to make the C API as much user-friendly as possible. This
is mostly due to the semantics of the C language. Since C++ is a more expressive
language, it was natural to try and bridge the gap using native C++ features.

There are three main features of the C++ bindings:
 - the `persistent_ptr<>` smart pointer,
 - the `transaction`, which comes in two flavours - scoped and closure,
 - the `p<>` property.

The main issue with the C API is the generic PMEMoid and its typed counterpart,
the TOID. For them to be conveniently used in transactions, a large set of
macros has been defined. This made using the generic pointer easier, yet still
unintuitive. In C++, the `persistent_ptr<>` template makes it a lot easier
providing well known smart pointer semantics and built-in transactions support.

The other drawback of the C API is the transaction semantics. Manual usage of
`setjmp` and `jmpbuf` is error prone, so they were once again wrapped in
macros. They themselves have issues with undefined values of automatic
variables (see the libpmemobj manpage for more details). The transactions
defined in the C++ bindings try to fix the inadequacies of their C counterparts.

The `p<>`, which is called the _persistent property_, was designed with
seamless persistent memory integration in mind. It is designed to be used with
basic types within classes, to signify that these members in fact reside in
persistent memory and need to be handled appropriately.

Please remember to take extra care when using _static class members_. They are
not stored in persistent memory, therefore their value will _not_ always be
consistent across subsequent executions or compilations of user applications.

If you find any issues or have suggestion about these bindings please file an
issue in https://github.com/pmem/libpmemobj-cpp/issues. There are also blog articles in
https://pmem.io/blog/ which you might find helpful.

Have fun!
The PMDK team

### Compiler notice ###
The C++ bindings require a C++11 compliant compiler, therefore the minimal
versions of GCC and Clang are 4.8.1 and 3.3 respectively. However the
pmem::obj::transaction::automatic class requires C++17, so
you need a more recent version for this to be available (GCC 6.1/Clang 3.7).
It is recommended to use these or newer versions of GCC or Clang.
A usage of the libpmemobj-cpp containers requires GCC >= 4.9.0 (see explanation
in the main README.md file).

### Standard notice ###
Please note that the C++11 standard, section 3.8, states that a valid
non-trivially default constructible object (in other words, not plain old data)
must be properly constructed in the lifetime of the application.
Libpmemobj, or any shared memory solution for that matter, does not
strictly adhere to that constraint.

We believe that in the future, languages that wish to support persistent memory
will need to alter their semantics to establish a defined behavior for objects
whose lifetimes exceed that of the application. In the meantime, the programs
that wish to use persistent memory will need to rely on compiler-defined
behavior.

Our library, and by extension these bindings, have been extensively tested in
g++, clang++ and MSVC++ to make sure that our solution is safe to use and
practically speaking implementation defined. The only exception to this rule is
the use of polymorphic types, which are notably forbidden when using C++
bindings.

### Important classes/functions ###

 * Transactional allocations - make_persistent.hpp
 * Transactional array allocations - make_persistent_array.hpp
 * Atomic allocations - make_persistent_atomic.hpp
 * Atomic array allocations - make_persistent_array_atomic.hpp
 * Resides on persistent memory property - [p](@ref pmem::obj::p)
 * Persistent smart pointer - [persistent_ptr](@ref pmem::obj::persistent_ptr)
 * Persistent memory transactions - [transaction](@ref pmem::obj::transaction)
 * Persistent memory resident mutex - [mutex](@ref pmem::obj::mutex)
 * Persistent memory pool - [pool](@ref pmem::obj::pool)
 * Defrag class - [defrag](@ref pmem::obj::defrag)

## Persistent containers ##

The C++ standard library containers collection is something that persistent
memory programmers may want to use. Containers manage the lifetime of held
objects through allocation/creation and deallocation/destruction with the use of
allocators. Implementing custom persistent allocator for C++ STL (Standard
Template Library) containers has two main downsides:

Implementation details:
 - STL containers do not use algorithms optimal from persistent memory programming point of view.
 - Persistent memory containers should have durability and consistency properties, while not every STL method guarantees strong exception safety.
 - Persistent memory containers should be designed with an awareness of fragmentation limitations.

Memory layout:
 - The STL does not guarantee that the container layout will remain unchanged in new library versions.

Due to these obstacles, the libpmemobj-cpp contains the set of custom,
implemented-from-scratch containers with optimized on-media layouts and
algorithms to fully exploit the potential and features of persistent memory.
These methods guarantee atomicity, consistency and durability. Besides specific
internal implementation details, libpmemobj-cpp persistent memory containers
have the well-known STL-like interface and they work with STL algorithms.

### Available containers ###

 * array with STL-like interface - [pmem::obj::array](@ref pmem::obj::array)
 * string with STL-like interface - [pmem::obj::string](@ref pmem::obj::basic_string)
 * vector with STL-like interface - [pmem::obj::vector](@ref pmem::obj::vector)
 * segment_vector with std::vector-like interface (no STL counterpart) - [pmem::obj::segment_vector](@ref pmem::obj::segment_vector)
 * concurrent_hash_map (no STL counterpart) - [pmem::obj::concurrent_hash_map](@ref pmem::obj::concurrent_hash_map)
