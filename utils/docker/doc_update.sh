#!/usr/bin/env bash
#
# Copyright 2018, Intel Corporation
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#
#     * Neither the name of the copyright holder nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -e

# clone repo
git clone https://github.com/doc-update/libpmemobj-cpp.git
cd libpmemobj-cpp
git remote add upstream https://github.com/igchor/libpmemobj-cpp

git config --local user.email "test"

git checkout master
git remote update
git rebase upstream/master

# build docs
mkdir build
cd build

cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF ..
make doc
cp -R doc/cpp_html ../..

cd ..

# checkout gh-pages and copy docs
git checkout -fb gh-pages upstream/gh-pages
git clean -df
cp -r ../cpp_html/* master/doxygen/

# add and push changes
git add -A

# This command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted)
git commit -m "doc: automatic gh-pages docs update" && true

git push -f https://${GITHUB_TOKEN}@github.com/doc-update/libpmemobj-cpp.git gh-pages

# Makes pull request. When there is already an open pr an error is thrown, which we ignore.
hub pull-request -f -b igchor:gh-pages -h doc-update:gh-pages -m "doc: automatic gh-pages docs update" && true

cd ..
rm -rf cpp_html libpmemobj-cpp;
