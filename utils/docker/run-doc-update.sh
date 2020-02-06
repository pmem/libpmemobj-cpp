#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

set -e

ORIGIN="https://${GITHUB_TOKEN}@github.com/pmem-bot/libpmemobj-cpp"
UPSTREAM="https://github.com/pmem/libpmemobj-cpp"

# Clone repo
git clone ${ORIGIN}
cd libpmemobj-cpp
git remote add upstream ${UPSTREAM}

git config --local user.name "pmem-bot"
git config --local user.email "pmem-bot@intel.com"

git checkout master
git remote update
git rebase upstream/master

# Build docs
mkdir build
cd build

cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_BENCHMARKS=OFF ..
make -j$(nproc) doc
cp -R doc/cpp_html ../..

cd ..

# Checkout gh-pages and copy docs
git checkout -fb gh-pages upstream/gh-pages
git clean -df
cp -r ../cpp_html/* master/doxygen/

# Add and push changes.
# git commit command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted).
git add -A
git commit -m "doc: automatic gh-pages docs update" && true
git push -f ${ORIGIN} gh-pages

# Makes pull request.
# When there is already an open PR or there are no changes an error is thrown, which we ignore.
hub pull-request -f -b pmem:gh-pages -h pmem-bot:gh-pages -m "doc: automatic gh-pages docs update" && true

exit 0
