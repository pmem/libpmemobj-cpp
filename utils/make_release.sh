#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

if [ -z "$VERSION_MAJOR" ]; then
    echo "VERSION_MAJOR is not set!"
    exit 1
fi

if [ -z "$VERSION_MINOR" ]; then
    echo "VERSION_MINOR is not set!"
    exit 1
fi

if [ -z "$REMOTE" ]; then
    echo "REMOTE is not set!"
    exit 1
fi

VER=$VERSION_MAJOR.$VERSION_MINOR
VERSION=$VERSION_MAJOR.$VERSION_MINOR

if [ -n "$VERSION_PATCH" ]; then
    VERSION=$VERSION.$VERSION_PATCH
fi

if [ -n "$VERSION_PRERELEASE" ]; then
    VERSION=$VERSION-$VERSION_PRERELEASE
fi

echo "VERSION: $VERSION"

ROOT_DIR=..

echo $VERSION > $ROOT_DIR/.version
git add $ROOT_DIR/.version

# Set VERSION vars in CMakeLists.txt
sed -i -r "s/set\(VERSION_MAJOR [0-9]+\)/set\(VERSION_MAJOR $VERSION_MAJOR\)/g" $ROOT_DIR/CMakeLists.txt
sed -i -r "s/set\(VERSION_MINOR [0-9]+\)/set\(VERSION_MINOR $VERSION_MINOR\)/g" $ROOT_DIR/CMakeLists.txt
if [ -n "$VERSION_PATCH" ]; then
    sed -i -r "s/set\(VERSION_PATCH [0-9]+\)/set\(VERSION_PATCH $VERSION_PATCH\)/g" $ROOT_DIR/CMakeLists.txt
else
    sed -i -r "s/set\(VERSION_PATCH [0-9]+\)/set\(VERSION_PATCH 0\)/g" $ROOT_DIR/CMakeLists.txt
fi
if [ -n "$VERSION_PRERELEASE" ]; then
    sed -i -r "s/(set\(VERSION_PATCH [0-9]+\))/\1\nset\(VERSION_PRERELEASE $VERSION_PRERELEASE\)/g" $ROOT_DIR/CMakeLists.txt
fi

# Create commit and tag
git add $ROOT_DIR/CMakeLists.txt
git commit -S -m "common: $VERSION release"
git tag -a -s -m "Libpmemobj-cpp Version $VERSION" $VERSION

# Undo temporary release changes
git rm $ROOT_DIR/.version
if [ -n "$VERSION_PRERELEASE" ]; then
    sed -i -r "/(set\(VERSION_PRERELEASE [0-9a-z]+\))/d" $ROOT_DIR/CMakeLists.txt
    git add $ROOT_DIR/CMakeLists.txt
fi
git commit -m "common: git versioning"

# XXX: check generated rpm/deb

# for major/minor releases
# if [ -z "$VERSION_PATCH" ]; then
#     git push $REMOTE HEAD:master $VERSION
#     git checkout -b stable-$VER
#     git push $REMOTE stable-$VER:stable-$VER
# else
#     git push $REMOTE HEAD:stable-$VER $VERSION
#     # XXX: create PR from stable to next stable
# fi

# XXX publish release

# XXX for major/minor releses: ask for vcpg update

# XXX generate compatibiity test