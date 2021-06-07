#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2020-2021, Intel Corporation

#
# download-scripts.sh - downloads specific version of
#			codecov's bash script to generate and upload reports.
#			It's useful, since they may break our coverage.
#

set -e

# master: Fix go regex (#436), 25.05.2021
CODECOV_VERSION="965008c97d649721850b9ff0de3f71e9b8adeb71"

if [ "${SKIP_SCRIPTS_DOWNLOAD}" ]; then
	echo "Variable 'SKIP_SCRIPTS_DOWNLOAD' is set; skipping scripts' download"
	exit
fi

mkdir -p /opt/scripts

# Download codecov's bash script
git clone https://github.com/codecov/codecov-bash
cd codecov-bash
git checkout ${CODECOV_VERSION}

git apply ../0001-fix-generating-gcov-files-and-turn-off-verbose-log.patch
mv -v codecov /opt/scripts/codecov

cd ..
rm -rf codecov-bash
