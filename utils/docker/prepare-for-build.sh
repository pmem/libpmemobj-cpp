#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2020, Intel Corporation

#
# prepare-for-build.sh - prepares environment within Docker container for
#		the build and stores functions common to run-* scripts in this dir.
#

set -e

function sudo_password() {
	echo ${USERPASS} | sudo -Sk $*
}

# this should be run only on CIs
if [ "${CI_RUN}" == "YES" ]; then
	sudo_password chown -R $(id -u).$(id -g) ${WORKDIR}
fi || true
