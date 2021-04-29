# Benchmarks

These benchmarks allow measuring some operations in the libpmemobj-cpp.

Currently following benchmarks are available:
- **concurrent_hash_map_insert_open**: this benchmark is used to measure time of inserting specified number of elements and time of runtime_initialize() in concurrent hash map.
- **radix_tree**: this benchmark is used to compare times of basic operations in radix_tree and std::map.
- **self_relative_pointer_assignment**: this benchmark is used to measure time of the assignment operator and the swap function for persistent_ptr and self_relative_ptr.
- **self_relative_pointer_get**: this benchmark is used to measure time of getting and changing a specified number of elements from a persistent array using self_relative_ptr and persistent_ptr.

# Running

**Warning:**
>All benchmarks shouldn't be run in a production environment, because they can cause problems with the content of the tested containers.

Each benchmark can need other input parameters. If you want to see usage of the certain benchmark, just run this benchmark's binary without any parameters, e.g.
```
./benchmark-self_relative_pointer_get
```