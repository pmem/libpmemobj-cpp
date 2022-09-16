#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2022, Intel Corporation

#
# install-pmdk.sh - installs libpmem & libpmemobj
#		and prepares DEB/RPM packages if possible (stored on docker image in /opt/).
#

set -e

PACKAGE_MANAGER=${1}

# common: 1.12.0 release, 24.05.2022
PMDK_VERSION="73d8f958e855904dc0776a7d77d0f0d3698a65b1"

if [ "${SKIP_PMDK_BUILD}" ]; then
	echo "Variable 'SKIP_PMDK_BUILD' is set; skipping building PMDK"
	exit
fi

git clone https://github.com/pmem/pmdk
cd pmdk
git checkout ${PMDK_VERSION}

make -j$(nproc) prefix=/opt/pmdk
sudo make install -j$(nproc) prefix=/opt/pmdk

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
