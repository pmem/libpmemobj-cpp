## Basic usage:

After [building](https://github.com/pmem/libpmemobj-cpp#how-to-build) the source, navigate to the `/build/examples/` directory. Operations include: `alloc`, `realloc`, `free`, and `print`.

 `$ ./example-array <file_name> [print|alloc|free|realloc] <array_name>`

## Options:

### Alloc:

`$ ./example-array <file_name> alloc <array_name> <size>`

Example:
```
$ ./example-array file alloc myArray 4
$ ./example-array file print myArray
    myArray = [0, 1, 2, 3]
```

### Realloc:

`$ ./example-array <file_name> realloc <array_name> <size>`

Example:
```
$ ./example-array file realloc myArray 7
$ ./example-array file print myArray
    myArray = [0, 1, 2, 3, 0, 0, 0]
```

### Free:

`$ ./example-array <file_name> free <array_name>`

Example:
```
$ ./example-array file free myArray
$ ./example-array file print myArray
    No array found with name: myArray
```

### Print:

`$ ./example-array <file_name> print <array_name>`

Example output can be seen in above example sections. 

