#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

#
# run-doc-update.sh - is called inside a Docker container,
#		to build docs for 'valid branches' and to create
#		a pull request with and update of doxygen files (on gh-pages).
#

set -e

if [[ -z "${DOC_UPDATE_GITHUB_TOKEN}" || -z "${DOC_UPDATE_BOT_NAME}" || -z "${DOC_REPO_OWNER}" ]]; then
	echo "To build documentation and upload it as a Github pull request, variables " \
		"'DOC_UPDATE_BOT_NAME', 'DOC_REPO_OWNER' and 'DOC_UPDATE_GITHUB_TOKEN' have to " \
		"be provided. For more details please read CONTRIBUTING.md"
	exit 0
fi

# Set up required variables
BOT_NAME=${DOC_UPDATE_BOT_NAME}
DOC_REPO_OWNER=${DOC_REPO_OWNER}
REPO_NAME=${REPO:-"libpmemobj-cpp"}
export GITHUB_TOKEN=${DOC_UPDATE_GITHUB_TOKEN} # export for hub command
REPO_DIR=$(mktemp -d -t libpmemobjcpp-XXX)
ARTIFACTS_DIR=$(mktemp -d -t ARTIFACTS-XXX)

# Only 'master' or 'stable-*' branches are valid
source $(dirname ${0})/valid-branches.sh
TARGET_BRANCH=${CI_BRANCH}
VERSION=${TARGET_BRANCHES[${TARGET_BRANCH}]}
if [ -z ${VERSION} ]; then
	echo "Target location for branch ${TARGET_BRANCH} is not defined."
	exit 1
fi

ORIGIN="https://${GITHUB_TOKEN}@github.com/${BOT_NAME}/${REPO_NAME}"
UPSTREAM="https://github.com/${DOC_REPO_OWNER}/${REPO_NAME}"

pushd ${REPO_DIR}
echo "Clone repo:"
git clone ${ORIGIN} ${REPO_DIR}
cd ${REPO_DIR}
git remote add upstream ${UPSTREAM}

git config --local user.name ${BOT_NAME}
git config --local user.email "${BOT_NAME}@intel.com"
hub config --global hub.protocol https

git remote update
git checkout -B ${TARGET_BRANCH} upstream/${TARGET_BRANCH}

echo "Build docs:"
mkdir -p ${REPO_DIR}/build
cd ${REPO_DIR}/build

cmake .. -DBUILD_TESTS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_BENCHMARKS=OFF
make -j$(nproc) doc
cp -r ${REPO_DIR}/build/doc/cpp_html ${ARTIFACTS_DIR}/

cd ${REPO_DIR}

# Checkout gh-pages and copy docs
GH_PAGES_NAME="gh-pages-for-${TARGET_BRANCH}"
git checkout -B ${GH_PAGES_NAME} upstream/gh-pages
git clean -dfx

# Clean old content, since some files might have been deleted
rm -rf ./${VERSION}
mkdir -p ./${VERSION}/doxygen/

cp -fr ${ARTIFACTS_DIR}/cpp_html/* ./${VERSION}/doxygen/

echo "Add and push changes:"
# git commit command may fail if there is nothing to commit.
# In that case we want to force push anyway (there might be open pull request with
# changes which were reverted).
git add -A
git commit -m "doc: automatic gh-pages docs update" && true
git push -f ${ORIGIN} ${GH_PAGES_NAME}

echo "Make or update pull request:"
# When there is already an open PR or there are no changes an error is thrown, which we ignore.
hub pull-request -f -b ${DOC_REPO_OWNER}:gh-pages -h ${BOT_NAME}:${GH_PAGES_NAME} -m "doc: automatic gh-pages docs update" && true

popd
