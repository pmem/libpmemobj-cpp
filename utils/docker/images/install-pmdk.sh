#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation

#
# install-pmdk.sh - installs libpmem & libpmemobj
#		and prepares DEB/RPM packages if possible (stored on docker image in /opt/).
#

set -e

if [ "${SKIP_PMDK_BUILD}" ]; then
	echo "Variable 'SKIP_PMDK_BUILD' is set; skipping building PMDK"
	exit
fi

PACKAGE_MANAGER=${1}

# master: Merge pull request #5304 from lukaszstolarczuk/update-build-rpm-sh, 07.09.2021
# Adds support for rpm's creation on Alma Linux
PMDK_VERSION="75d3cd27589c7e0823d48b32353253975752666b"

git clone https://github.com/pmem/pmdk
cd pmdk
git checkout ${PMDK_VERSION}

# Don't generate docs, they are reundant for us
make DOC=n -j$(nproc) prefix=/opt/pmdk
sudo make DOC=n install -j$(nproc) prefix=/opt/pmdk

# Do not create nor test any packages if PACKAGE_MANAGER is not set
[[ -z "${PACKAGE_MANAGER}" ]] && exit 0

sudo mkdir /opt/pmdk-pkg
make -j$(nproc) BUILD_PACKAGE_CHECK=n "${PACKAGE_MANAGER}"

if [ "${PACKAGE_MANAGER}" = "dpkg" ]; then
	sudo mv dpkg/*.deb /opt/pmdk-pkg/
elif [ "${PACKAGE_MANAGER}" = "rpm" ]; then
	sudo mv rpm/x86_64/*.rpm /opt/pmdk-pkg/
fi

cd ..
rm -rf pmdk
