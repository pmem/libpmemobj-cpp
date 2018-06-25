#!/usr/bin/env bash
#
# Copyright 2016-2018, Intel Corporation
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
# run-build.sh - is called inside a Docker container; prepares the environment
#                and starts a build of libpmemobj-cpp.
#

set -e

function cleanup() {
	find . -name ".coverage" -exec rm {} \;
	find . -name "coverage.xml" -exec rm {} \;
	find . -name "*.gcov" -exec rm {} \;
	find . -name "*.gcda" -exec rm {} \;
}

function test_command() {
	if [ "$COVERAGE" = "1" ]; then
		ctest --output-on-failure -E "_memcheck|_drd|_helgrind|_pmemcheck"
		bash <(curl -s https://codecov.io/bash)
		cleanup
	else
		ctest --output-on-failure
	fi
}

cd $WORKDIR
INSTALL_DIR=/tmp/libpmemobj-cpp

mkdir $INSTALL_DIR

mkdir build
cd build

CC=clang CXX=clang++ \
cmake .. -DDEVELOPER_MODE=1 \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE \
			-DUSE_LLVM_LIBCPP=1 \
			-DLIBCPP_LIBDIR=$LIBCPP_LIBDIR \
			-DLIBCPP_INCDIR=$LIBCPP_INCDIR

make -j2
test_command
make install

cd ..
rm -r build

mkdir build
cd build

CC=clang CXX=clang++ \
cmake .. -DDEVELOPER_MODE=1 \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE \
			-DUSE_LLVM_LIBCPP=0

make -j2
test_command
make install

cd ..
rm -r build

mkdir build
cd build

CC=gcc CXX=g++ \
cmake .. -DDEVELOPER_MODE=1 \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE \
			-DUSE_LLVM_LIBCPP=0

make -j2
test_command
make install

cd ..
rm -r build

mkdir build
cd build

CC=gcc CXX=g++ \
cmake .. -DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE \
			-DUSE_LLVM_LIBCPP=0

make -j2
test_command
make install

cd ..
rm -r build

mkdir build
cd build

cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
		-DCPACK_GENERATOR=$PACKAGE_MANAGER

make -j2 package

if [ $PACKAGE_MANAGER = "deb" ]; then
      echo $USERPASS | sudo -S dpkg -i libpmemobj++*.deb
elif [ $PACKAGE_MANAGER = "rpm" ]; then
      echo $USERPASS | sudo -S rpm -i libpmemobj++*.rpm
fi

#XXX: verify installed package - try to compile some program/example

cd ..
rm -rf build

rm -r $INSTALL_DIR
