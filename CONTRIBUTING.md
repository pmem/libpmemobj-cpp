# Contributing to libpmemobj-cpp

- [Opening New Issues](#opening-new-issues)
- [Code Style](#code-style)
- [Submitting Pull Requests](#submitting-pull-requests)
- [Implementing persistent containers](#implementing-persistent-containers)
- [Configuring GitHub fork](#configuring-github-fork)

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

# Implementing persistent containers

When developing a persistent container make sure you follow the rules and steps described here.

## Steps
1. Create container implementation
2. Create doc_snippet/example
3. Add TEST_CONTAINER CMake option for enabling/disabling container's testing.
4. Add tests (+ if possible, enable libcxx tests in tests/external/libcxx)

## Requirements:
* Constructors should check whether the container is being constructed on pmem and within a transaction.
  If not, appropriate exception should be thrown.
* If any operation has to or is not allowed to be called inside of a transaction there should be a proper check for that.
* Operations which modify the entire container (like operator=, clear(), etc.) should be transactional.
* If any operation inside destructor can fail there should be a separate method like free_data/destroy.
* Each complex container should have a mechanism for checking layout compatibility. One way is to store size of key and value on pmem and compare it with sizeof(value_type) after pool reopen.
* Padding should always be explicit.
* If any object from standard library is being stored on pmem its size should be verified by static_assert.
* Public methods should be documented using doxygen comments.

## Optional features:
* for_each_ptr method for defragmentation purposes.
* Containers with lower memory overhead (like vector) can implement some heuristic for verifying layout (like comparing pmemobj_alloc_usable_size with expected capacity).

# Configuring GitHub fork

To build and submit documentation as an automatically generated pull request,
the repository has to be properly configured.

* [Personal access token](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token) for GitHub account has to be generated.
  * Such personal access token has to be set in in GitHub repository's
  [secrets](https://docs.github.com/en/actions/configuring-and-managing-workflows/creating-and-storing-encrypted-secrets)
  as `DOC_UPDATE_GITHUB_TOKEN` variable.

* `DOC_UPDATE_BOT_NAME` secret variable has to be set. In most cases it will be
  the same as GitHub account name.

* `DOC_REPO_OWNER` secret variable has to be set. Name of GitHub account,
  which will be target to make an automatic pull request with documentation.
  In most cases it will be the same as GitHub account name.

To enable automatic images pushing to GitHub Container Registry, following variables:

* `CONTAINER_REG` existing environment variable (defined in workflow files, in .github/ directory)
  has to be updated to contain proper GitHub Container Registry address (to forking user's container registry),

* `GH_CR_USER` secret variable has to be set up - an account (with proper permissions) to publish
  images to the Container Registry (tab **Packages** in your GH profile/organization).

* `GH_CR_PAT` secret variable also has to be set up - Personal Access Token
  (with only read & write packages permissions), to be generated as described
  [here](https://docs.github.com/en/free-pro-team@latest/github/authenticating-to-github/creating-a-personal-access-token#creating-a-token)
  for selected account (user defined in above variable).
