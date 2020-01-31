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
