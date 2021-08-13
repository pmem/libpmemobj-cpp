#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2021, Intel Corporation

#
# install-valgrind.sh - installs valgrind for persistent memory
#

set -e

if [ "${SKIP_VALGRIND_BUILD}" ]; then
	echo "Variable 'SKIP_VALGRIND_BUILD' is set; skipping building valgrind (pmem's fork)"
	exit
fi

git clone --recursive https://github.com/lukaszstolarczuk/valgrind
cd valgrind
# pmem-3.17
git checkout pmem-3.17

./autogen.sh
./configure --prefix=/usr
make -j$(nproc)
sudo make -j$(nproc) install
cd ..
rm -rf valgrind
