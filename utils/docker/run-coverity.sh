#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

#
# run-coverity.sh - runs the Coverity scan build and send report to coverity.com
#

set -e

# Run coverity build only on upstream repository and check for req. vars
if [[ "${CI_REPO_SLUG}" != "${GITHUB_REPO}" ]]; then
	echo
	echo "Skipping Coverity build: it doesn't run on forked repos."
	exit 0
fi
if [[ -z "${COVERITY_SCAN_NOTIFICATION_EMAIL}"
     || -z "${COVERITY_SCAN_TOKEN}" ]]; then
	echo
	echo "Skipping Coverity build:"\
		"COVERITY_SCAN_TOKEN=\"${COVERITY_SCAN_TOKEN}\" or"\
		"COVERITY_SCAN_NOTIFICATION_EMAIL="\
		"\"${COVERITY_SCAN_NOTIFICATION_EMAIL}\" are not set."
	exit 0
fi

# Prepare build environment
source $(dirname ${0})/prepare-for-build.sh

CERT_FILE=/etc/ssl/certs/ca-certificates.crt
TEMP_CF=$(mktemp)
cp ${CERT_FILE} ${TEMP_CF}

# Download Coverity certificate
echo -n | openssl s_client -connect scan.coverity.com:443 | \
	sed -ne '/-BEGIN CERTIFICATE-/,/-END CERTIFICATE-/p' | \
	tee -a ${TEMP_CF}

sudo_password mv ${TEMP_CF} ${CERT_FILE}

echo "Prepare CMake build"
mkdir ${WORKDIR}/build
cd ${WORKDIR}/build
PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/pmdk/lib/pkgconfig/ cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_DOC=OFF

# The 'travisci_build_coverity_scan.sh' script requires the following
# environment variables to be set:
export COVERITY_SCAN_PROJECT_NAME="${GITHUB_REPO}"
export COVERITY_SCAN_BRANCH_PATTERN="master"
export COVERITY_SCAN_BUILD_COMMAND="make -j$(nproc)"
export TRAVIS_BRANCH=${CI_BRANCH}
[ "${CI_EVENT_TYPE}" == "pull_request" ] && export TRAVIS_PULL_REQUEST="true"

echo "Run the Coverity scan"

# XXX: Patch the Coverity script.
# Recently, this script regularly exits with an error, even though
# the build is successfully submitted.  Probably because the status code
# is missing in response, or it's not 201.
# Changes:
# 1) change the expected status code to 200 and
# 2) print the full response string.
#
# This change should be reverted when the Coverity script is fixed.
#
# The previous version was:
# curl -s https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh | bash

wget https://scan.coverity.com/scripts/travisci_build_coverity_scan.sh
patch < ../utils/docker/0001-travis-fix-travisci_build_coverity_scan.sh.patch
bash ./travisci_build_coverity_scan.sh
