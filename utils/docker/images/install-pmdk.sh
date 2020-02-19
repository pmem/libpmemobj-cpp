#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2020, Intel Corporation

#
# install-pmdk.sh - installs libpmem & libpmemobj
#

set -e

PACKAGE_MANAGER=$1

# tag: 1.8, 31.01.2020
PMDK_VERSION="1.8"

git clone https://github.com/pmem/pmdk
cd pmdk
git checkout $PMDK_VERSION

make -j$(nproc) prefix=/opt/pmdk
sudo make install -j$(nproc) prefix=/opt/pmdk

# Do not create nor test any packages if PACKAGE_MANAGER is not set.
[ "$PACKAGE_MANAGER" == "" ] && exit 0

sudo mkdir /opt/pmdk-pkg
make -j$(nproc) BUILD_PACKAGE_CHECK=n "$PACKAGE_MANAGER"

if [ "$PACKAGE_MANAGER" = "dpkg" ]; then
	sudo mv dpkg/*.deb /opt/pmdk-pkg/
elif [ "$PACKAGE_MANAGER" = "rpm" ]; then
	sudo mv rpm/x86_64/*.rpm /opt/pmdk-pkg/
fi

cd ..
rm -rf pmdk
