#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# install-valgrind.sh - installs valgrind for persistent memory
#

set -e

git clone --recursive https://github.com/pmem/valgrind.git
cd valgrind
# pmem-3.15: Merge pull request #73 from kkajrewicz/fix-memcheck
git checkout c27a8a2f973414934e63f1e94bc84c0a580e3840
./autogen.sh
./configure --prefix=/usr
make -j$(nproc)
make -j$(nproc) install
cd ..
rm -rf valgrind
