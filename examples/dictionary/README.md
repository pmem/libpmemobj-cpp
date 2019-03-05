## Basic usage:

After [building](https://github.com/pmem/libpmemobj-cpp#how-to-build) the source, navigate to the `/build/examples/` directory. Operations include: `print`, `print_debug`, `insert_generate`, `insert`, `lookup`, `delete` and `delete_all`.

 `$ ./example-dictionary <file_name> insert_generate <number> <max_len>`
 `$ ./example-dictionary <file_name> insert <word_1> <word_2> ... <word_n>`
 `$ ./example-dictionary <file_name> lookup <word>`
 `$ ./example-dictionary <file_name> delete <word_1> <word_2> ... <word_n>`
 `$ ./example-dictionary <file_name> delete_all`
 `$ ./example-dictionary <file_name> print`
 `$ ./example-dictionary <file_name> print_debug`

## Options:

### print:

`$ ./example-dictionary <file_name> insert_generate <number> <max_len>`

Example:
```
$ ./example-dictionary file insert_generate 10 3
    Inserting word "G"...
    Inserting word "D"...
    Inserting word "Jmu"...
    Inserting word "UI"...
    Inserting word "a"...
    Inserting word "DRW"...
    Inserting word "CP7"...
    Inserting word "eD"...
    Inserting word "Rbz"...
    Inserting word "xVt"...
$ ./example-dictionary file print
    There are 10 element(s) in dictionary (listed in alphabetic order):
    CP7
    D
    DRW
    G
    Jmu
    Rbz
    UI
    a
    eD
    xVt
```

### insert:

`$ ./example-array <file_name> insert <word_1> <word_2> ... <word_n>`

Example:
```
$ ./example-dictionary file insert ab ac abc
$ ./example-dictionary file print_debug
    There are 3 element(s) in dictionary (listed in alphabetic order):
    current node:
                word: a
                prefix: a
                is_word: 0
                children(s): b c 
                parent: nullptr (this is root node)
    current node:
                word: ab
                prefix: b
                is_word: 1
                children(s): c 
                parent: a(root)

    current node:
                word: abc
                prefix: c
                is_word: 1
                children(s): (leaf)
                parent: b

    current node:
                word: ac
                prefix: c
                is_word: 1
                children(s): (leaf)
                parent: a(root)
```

### lookup:

`$ ./example-dictionary <file_name> lookup <word>`

Example:
```
$ ./example-dictionary file insert ab ac abc
$ ./example-dictionary file lookup ab
    1
$ ./example-dictionary file lookup abx
    0
```

### delete:
`$ ./example-dictionary <file_name> delete <word_1> <word_2> ... <word_n>`
Example:
```
$ ./example-dictionary file insert ab ac abc
$ ./example-dictionary file delete ab ac
$ ./example-dictionary file print_debug
    There are 1 element(s) in dictionary (listed in alphabetic order):
    current node:
                word: abc
                prefix: abc
                is_word: 1
                children(s): (leaf)
                parent: nullptr (this is root node)
```

### delete_all
`$ ./example-dictionary <file_name> delete_all`
Example:
```
./example-dictionary file insert_generate 10 3
    Inserting word "G"...
    Inserting word "D"...
    Inserting word "Jmu"...
    Inserting word "UI"...
    Inserting word "a"...
    Inserting word "DRW"...
    Inserting word "CP7"...
    Inserting word "eD"...
    Inserting word "Rbz"...
    Inserting word "xVt"...
$ ./example-dictionary file delete_all
$ ./example-dictionary file print
    There are 0 element(s) in dictionary (listed in alphabetic order):
```

### print
`$ ./example-dictionary <file_name> print`

Example output can be seen in above example sections. 

### print_debug
`$ ./example-dictionary <file_name> print_debug`

Example output can be seen in above example sections. 
