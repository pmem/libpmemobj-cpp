#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2022, Intel Corporation

# XXX: this script is still missing:
# - updating CMakeLists.txt date
# - publishing release to github

check_input() {
    read input
    if [[ "$input" != "y" ]]; then
        exit 1
    fi
}

echo "Update and verify ChangeLog. Is it correct? [y/n]"
check_input

if [ -z "$VERSION_MAJOR" ]; then
    echo "VERSION_MAJOR is not set!"
    exit 1
fi

if [ -z "$VERSION_MINOR" ]; then
    echo "VERSION_MINOR is not set!"
    exit 1
fi

if [ -z "$VERSION_PATCH" ]; then
    VERSION_PATCH=0
fi

VER=$VERSION_MAJOR.$VERSION_MINOR
VERSION=$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH

if [ -n "$VERSION_PRERELEASE" ]; then
    VERSION=$VERSION-$VERSION_PRERELEASE
fi

echo "VERSION: $VERSION"

ROOT_DIR=$(dirname $0)/..

echo $VERSION > $ROOT_DIR/.version
git add $ROOT_DIR/.version

# Set VERSION vars in CMakeLists.txt
sed -i -r "s/set\(VERSION_MAJOR [0-9]+\)/set\(VERSION_MAJOR $VERSION_MAJOR\)/g" $ROOT_DIR/CMakeLists.txt
sed -i -r "s/set\(VERSION_MINOR [0-9]+\)/set\(VERSION_MINOR $VERSION_MINOR\)/g" $ROOT_DIR/CMakeLists.txt
sed -i -r "s/set\(VERSION_PATCH [0-9]+\)/set\(VERSION_PATCH $VERSION_PATCH\)/g" $ROOT_DIR/CMakeLists.txt
if [ -n "$VERSION_PRERELEASE" ]; then
    sed -i -r "s/(set\(VERSION_PATCH [0-9]+\))/\1\nset\(VERSION_PRERELEASE $VERSION_PRERELEASE\)/g" $ROOT_DIR/CMakeLists.txt
fi

# XXX: update CMakeLists.txt date if needed

# Create commit and tag
git add $ROOT_DIR/CMakeLists.txt
git commit -S -m "common: $VERSION release"
git tag -a -s -m "Libpmemobj-cpp Version $VERSION" $VERSION

# Undo temporary release changes
git rm $ROOT_DIR/.version
if [ -n "$VERSION_PRERELEASE" ]; then
    sed -i -r "/(set\(VERSION_PRERELEASE [0-9a-zA-Z]+\))/d" $ROOT_DIR/CMakeLists.txt
    git add $ROOT_DIR/CMakeLists.txt
fi
git commit -m "common: git versioning"

echo "Check generated rpm/deb. Are they correct? [y/n]"
check_input

echo "Now follow rest of the steps from doc/RELEASE_STEPS.md"
