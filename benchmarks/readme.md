# Benchmarks

These benchmarks allow measuring some operations in the libpmemobj-cpp.

Currently following benchmarks are available:
- **concurrent_hash_map_insert_open**: this benchmark is used to measure time of inserting specified number of elements and time of `runtime_initialize()` in concurrent hash map.
- **radix_tree**: this benchmark is used to compare times of basic operations in radix_tree and std::map.
- **self_relative_pointer_assignment**: this benchmark is used to measure time of the assignment operator and the swap function for persistent_ptr and self_relative_ptr.
- **self_relative_pointer_get**: this benchmark is used to measure time of accessing and changing a specified number of elements from a persistent array using self_relative_ptr and persistent_ptr.

## Compiling

Follow build steps for your OS (as described in top-level README), just make sure that the BUILD_BENCHMARKS options is ON.

To build all benchmarks:
```sh
$ cd benchmarks
$ make
```

To build a certain benchmark:
```sh
$ cd benchmarks
$ make benchmark-name_of_benchmark
```

## Running

**Warning:**
>These benchmarks shouldn't be run in a production environment, because they can remove/modify existing data in the tested containers.

Each benchmark can require various input parameters. If you want to see the usage of the certain benchmark, just run this benchmark's binary without any parameters, e.g.
```sh
$ ./benchmark-radix_tree
```
For example radix_tree benchmark needs following parameters: file_name \[count] \[batch_size] \[sample_size]. Example execution:
```sh
$ ./benchmark-radix_tree my_pool 20000 200 2000
```
