## libpmemobj-cpp release steps

This document contains all the steps required to make a new release of libpmemobj-cpp.

\#define $VERSION = current full version (e.g. 1.0.2); $VER = major+minor only version (e.g. 1.0)

Run utils/make_release.sh and follow all the steps.

Publish changes:
- for major/minor release:
  - git push upstream HEAD:master $VERSION
  - git checkout stable-$VER && git push upstream stable-$VER:stable-$VER
- for patch release:
  - git push upstream HEAD:stable-$VER $VERSION
  - create PR from stable-$VER to next stable (or master, if release is from the most recent stable branch)

Publish package and make it official:
- go to [GitHub's releases tab](https://github.com/pmem/libpmemobj-cpp/releases/new):
  - tag version: $VERSION, release title: Libpmemobj-cpp Version $VERSION,
    description: copy entry from ChangeLog and format it with no tabs and no characters limit in line
- announce the release on pmem group and on pmem slack channel(s)

Later, for major/minor release:
- update library version in [vcpkg](https://github.com/microsoft/vcpkg/blob/master/ports/libpmemobj-cpp) - file an inssue for their maintainers
- add new compatibility tests (for new version) in tests/compatibility/CMakeLists.txt, on stable-$VER branch
- once gh-pages contains new documentation:
 - add there (in index.md) v.$VER section in Doxygen docs links
 - update "Releases' support status" table (older releases' statuses as well, if needed)
