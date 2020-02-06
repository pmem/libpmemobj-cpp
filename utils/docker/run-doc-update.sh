#!/usr/bin/env bash
#
# Copyright 2018-2020, Intel Corporation
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

source `dirname $0`/valid-branches.sh

BOT_NAME="pmem-bot"
USER_NAME="pmem"
REPO_NAME="libpmemobj-cpp"

ORIGIN="https://${GITHUB_TOKEN}@github.com/${BOT_NAME}/${REPO_NAME}"
UPSTREAM="https://github.com/pmem/${REPO_NAME}"
# master or stable-* branch
TARGET_BRANCH=${TRAVIS_BRANCH}
VERSION=${TARGET_BRANCHES[$TARGET_BRANCH]}

if [ -z $VERSION ]; then
	echo "Target location for branch $TARGET_BRANCH is not defined."
	exit 1
fi

# Clone repo
git clone ${ORIGIN}
cd ${REPO_NAME}
git remote add upstream ${UPSTREAM}

git config --local user.name ${BOT_NAME}
git config --local user.email "${BOT_NAME}@intel.com"

git remote update
git checkout -B ${TARGET_BRANCH} upstream/${TARGET_BRANCH}

# Build docs
mkdir build
cd build

cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF ..
make doc
cp -R doc/cpp_html ../..

cd ..

# Checkout gh-pages and copy docs
GH_PAGES_NAME="gh-pages-for-${TARGET_BRANCH}"
git checkout -B $GH_PAGES_NAME upstream/gh-pages
git clean -dfx

# Clean old content, since some files might have been deleted
rm -r ./$VERSION
mkdir ./$VERSION

cp -r ../cpp_html/* ./$VERSION/doxygen/

# Add and push changes.
# git commit command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted).
git add -A
git commit -m "doc: automatic gh-pages docs update" && true
git push -f ${ORIGIN} $GH_PAGES_NAME

# Makes pull request.
# When there is already an open PR or there are no changes an error is thrown, which we ignore.
hub pull-request -f -b ${USER_NAME}:gh-pages -h ${BOT_NAME}:${GH_PAGES_NAME} -m "doc: automatic gh-pages docs update" && true

exit 0
