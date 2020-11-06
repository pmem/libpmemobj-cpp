#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# prepare-for-build.sh - prepares environment for the build
#		(on CI, or when ./build.sh used, it happens inside a Docker container)
#		and stores functions common to run-* scripts in this dir.
#

set -e

if [[ -z "${WORKDIR}" ]]; then
	echo "ERROR: The variable WORKDIR has to contain a path to the root " \
		"of this project - 'build' sub-directory will be created there."
	exit 1
fi

INSTALL_DIR=/tmp/libpmemobj-cpp
EXAMPLE_TEST_DIR=/tmp/build_example

function sudo_password() {
	echo ${USERPASS} | sudo -Sk $*
}

function workspace_cleanup() {
	echo "Cleanup build dirs:"

	cd ${WORKDIR}
	rm -rf ${WORKDIR}/build
	rm -rf ${EXAMPLE_TEST_DIR}
	rm -rf ${INSTALL_DIR}
}

# this should be run only on CIs
if [ "${CI_RUN}" == "YES" ]; then
	sudo_password chown -R $(id -u).$(id -g) ${WORKDIR}
fi || true
