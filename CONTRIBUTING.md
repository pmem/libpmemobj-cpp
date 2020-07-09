# Contributing to libpmemobj-cpp

- [Opening New Issues](#opening-new-issues)
- [Code Style](#code-style)
- [Submitting Pull Requests](#submitting-pull-requests)

# Opening New Issues

Please log bugs or suggestions as [GitHub issues](https://github.com/pmem/libpmemobj-cpp/issues).
Details such as OS and PMDK version are always appreciated.

# Code Style

* See `.clang-format` file in the repository for details
* Indent with tabs (width: 8)
* Max 90 chars per line
* Space before '*' and '&' (rather than after)

If you want to check and format your source code properly you can use CMake's `DEVELOPER_MODE`
and `CHECK_CPP_STYLE` options. When enabled additional checks are switched on
(cppstyle, whitespace and license).

```sh
cmake .. -DDEVELOPER_MODE=ON -DCHECK_CPP_STYLE=ON
```

If you just want to format your code you can make adequate target:
```sh
make cppformat
```

**NOTE**: We're using specific clang-format - version exactly **9.0** is required.

# Submitting Pull Requests

We take outside code contributions to `libpmemobj-cpp` through GitHub pull requests.

**NOTE: If you do decide to implement code changes and contribute them,
please make sure you agree your contribution can be made available under the
[BSD-style License used for libpmemobj-cpp](LICENSE).**

**NOTE: Submitting your changes also means that you certify the following:**

```
Developer's Certificate of Origin 1.1

By making a contribution to this project, I certify that:

(a) The contribution was created in whole or in part by me and I
    have the right to submit it under the open source license
    indicated in the file; or

(b) The contribution is based upon previous work that, to the best
    of my knowledge, is covered under an appropriate open source
    license and I have the right under that license to submit that
    work with modifications, whether created in whole or in part
    by me, under the same open source license (unless I am
    permitted to submit under a different license), as indicated
    in the file; or

(c) The contribution was provided directly to me by some other
    person who certified (a), (b) or (c) and I have not modified
    it.

(d) I understand and agree that this project and the contribution
    are public and that a record of the contribution (including all
    personal information I submit with it, including my sign-off) is
    maintained indefinitely and may be redistributed consistent with
    this project or the open source license(s) involved.
```

In case of any doubt, the gatekeeper may ask you to certify the above in writing,
i.e. via email or by including a `Signed-off-by:` line at the bottom
of your commit comments.

To improve tracking of who is the author of the contribution, we kindly ask you
to use your real name (not an alias) when committing your changes to PMEMKV:
```
Author: Random J Developer <random@developer.example.org>
```
