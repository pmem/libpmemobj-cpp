#!/usr/bin/env bash
#
# Copyright 2018, Intel Corporation
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

git clone https://github.com/pmem/pmdk
cd pmdk
git checkout 1.5.1

sudo make EXTRA_CFLAGS="-DUSE_COW_ENV" -j2 install prefix=/opt/pmdk

sudo mkdir /opt/pmdk-pkg

# Download and save pmdk-1.4 packages
if [ "$1" = "dpkg" ]; then
	wget https://github.com/pmem/pmdk/releases/download/1.4/pmdk-1.4-dpkgs.tar.gz
	tar -xzf pmdk-1.4-dpkgs.tar.gz
	sudo mv *.deb /opt/pmdk-pkg/
elif [ "$1" = "rpm" ]; then
	wget https://github.com/pmem/pmdk/releases/download/1.4/pmdk-1.4-rpms.tar.gz
	tar -xzf pmdk-1.4-rpms.tar.gz
	sudo mv x86_64/*.rpm /opt/pmdk-pkg/
fi

cd ..
rm -rf pmdk
