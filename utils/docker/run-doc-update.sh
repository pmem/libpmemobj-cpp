#!/usr/bin/env bash
#
# Copyright 2018-2022, Intel Corporation
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

if [[ -z "${DOC_UPDATE_GITHUB_TOKEN}" ]]; then
	echo "To build documentation and upload it as a Github pull request, variable " \
		"'DOC_UPDATE_GITHUB_TOKEN' has to be provided."
	exit 1
fi

if [[ -z "${WORKDIR}" ]]; then
	echo "ERROR: The variable WORKDIR has to contain a path to the root " \
		"of this project - 'build' sub-directory may be created there."
	exit 1
fi

BOT_NAME=${DOC_UPDATE_BOT_NAME:-"pmem-bot"}
DOC_REPO_OWNER=${DOC_REPO_OWNER:-"pmem"}
DOC_REPO_NAME=${DOC_REPO_NAME:-"${DOC_REPO_OWNER}.github.io"}
export GITHUB_TOKEN=${DOC_UPDATE_GITHUB_TOKEN} # export for hub command
DOC_REPO_DIR=$(mktemp -d -t pmem_io-XXX)
ARTIFACTS_DIR=$(mktemp -d -t ARTIFACTS-XXX)

# Determine docs location directory, based on the branch (for CI builds: 'master' or 'vX.Y')
if [[ ${CI_BRANCH} == stable-* ]]; then
	DOCS_TARGET_DIR=$(echo ${CI_BRANCH} | awk -F 'stable-' '/stable-/{printf "v"} {print $(NF)}')
else
	DOCS_TARGET_DIR=${CI_BRANCH}
fi

ORIGIN="https://${GITHUB_TOKEN}@github.com/${BOT_NAME}/${DOC_REPO_NAME}"
UPSTREAM="https://github.com/${DOC_REPO_OWNER}/${DOC_REPO_NAME}"

echo "Build docs"
pushd ${WORKDIR}
mkdir -p build
pushd build

cmake -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_BENCHMARKS=OFF -DBUILD_DOC=ON ..
make -j$(nproc) doc
cp -R doc/cpp_html ${ARTIFACTS_DIR}
popd
popd

echo "Clone pmem.io repo (with web content and our docs):"
git clone --depth=1 ${ORIGIN} ${DOC_REPO_DIR}
pushd ${DOC_REPO_DIR}
git remote add upstream ${UPSTREAM}
git fetch upstream

git config --local user.name ${BOT_NAME}
git config --local user.email "${BOT_NAME}@intel.com"
hub config --global hub.protocol https

echo "Checkout new branch (based on 'main') for PR"
DOCS_BRANCH_NAME="libpmemobj-cpp-${DOCS_TARGET_DIR}-docs-update"
git checkout -B ${DOCS_BRANCH_NAME} upstream/main
git clean -dfx

DOCS_CONTENT_DIR="./content/libpmemobj-cpp/${DOCS_TARGET_DIR}/"
echo "Clean old content, since some files might have been deleted"
rm -rf ${DOCS_CONTENT_DIR}
mkdir -p ${DOCS_CONTENT_DIR}/doxygen/

echo "Copy all content"
cp -r ${ARTIFACTS_DIR}/cpp_html/* ${DOCS_CONTENT_DIR}/doxygen/

echo "Add and push changes"
# git commit command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted).
git add -A
git commit -m "libpmemobj-cpp: automatic docs update for '${CI_BRANCH}'" && true
git push -f ${ORIGIN} ${DOCS_BRANCH_NAME}

echo "Make a Pull Request"
# When there is already an open PR or there are no changes an error is thrown, which we ignore.
hub pull-request -f -b ${DOC_REPO_OWNER}:main -h ${BOT_NAME}:${DOCS_BRANCH_NAME} \
	-m "libpmemobj-cpp: automatic docs update for '${CI_BRANCH}'" && true

popd
