# libpmemobj-cpp release steps

This document contains all the steps required to make a new release of libpmemobj-cpp.

Make a release locally:
- add an entry to ChangeLog, remember to change the day of the week in the release date
  - for major/minor releases mention compatibility with the previous release
- echo $VERSION > .version
- git add .version
- update project's version in top-level CMakeLists.txt
- git commit -a -S -m "common: $VERSION release"
- git tag -a -s -m "Libpmemobj-cpp Version $VERSION" $VERSION

# Make a package:
# - git archive --prefix="pmdk-$VERSION/" -o pmdk-$VERSION.tar.gz $VERSION
# - uncompress the created archive in a new directory and create the final package:
# ```
#   $ cd pmdk-$VERSION
#   $ make doc
#   $ touch .skip-doc
#   $ cd ..
#   $ tar czf pmdk-$VERSION.tar.gz pmdk-$VERSION/ --owner=root --group=root
# ```
# - verify the created archive (uncompress & build one last time)
# - gpg --armor --detach-sign pmdk-$VERSION.tar.gz

Undo temporary release changes:
- git rm .version
- git commit --reset-author -m "common: git versioning"

Publish changes:
- for major/minor release:
  - git push upstream HEAD:master $VERSION
  - create and push to upstream stable-$VERSION branch
  - create PR from stable-$VERSION to master
- for patch release:
  - git push upstream HEAD:stable-$VER $VERSION
  - create PR from stable-$VER to next stable (or master, if release is from most recent stable branch)

Publish package and make it official:
- go to https://github.com/pmem/libpmemobj-cpp/releases/new:
  - tag version: $VERSION, release title: Libpmemobj-cpp Version $VERSION, description: copy entry from ChangeLog and format it with no tabs and no characters limit in line
# - upload pmdk-$VERSION.tar.gz & pmdk-$VERSION.tar.gz.asc
- announce the release on pmem group

Later, for major/minor release:
- bump version of Docker images (build.sh, build-image.sh, push-image.sh, pull-or-rebuild-image.sh) to $VERSION+1 on master branch
- add new branch to valid-branches.sh on stable-$VERSION branch
- once gh-pages contains new documentation, add $VERSION section in Doxygen docs links and in "Releases' support status" table (and update any status if needed) on gh-pages
