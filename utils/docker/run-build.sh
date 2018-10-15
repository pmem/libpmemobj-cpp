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
		if [[ "$2" == "llvm" ]]; then
			gcovexe="llvm-cov gcov"
		else
			gcovexe="gcov"
		fi

		ctest --output-on-failure -E "_memcheck|_drd|_helgrind|_pmemcheck" --timeout 540
		bash <(curl -s https://codecov.io/bash) -c -F $1 -x "$gcovexe"
		cleanup
	else
		ctest --output-on-failure --timeout 540
	fi
}

function compile_example_standalone() {
	rm -rf /tmp/build_example
	mkdir /tmp/build_example
	cd /tmp/build_example

	cmake $WORKDIR/examples/$1

	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	make
	cd -
}

cd $WORKDIR
INSTALL_DIR=/tmp/libpmemobj-cpp

mkdir $INSTALL_DIR

###############################################################################
# BUILD tests_clang_release llvm
###############################################################################
printf "\n$(tput setaf 1)$(tput setab 7)BUILD tests_clang_release llvm START$(tput sgr 0)\n"
mkdir build
cd build

PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
CC=clang CXX=clang++ \
cmake .. -DDEVELOPER_MODE=1 \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE

make -j2
test_command tests_clang_release llvm

cd ..
rm -r build
printf "$(tput setaf 1)$(tput setab 7)BUILD tests_clang_release llvm END$(tput sgr 0)\n\n"

###############################################################################
# BUILD tests_clang_debug_cpp17 llvm
###############################################################################
printf "\n$(tput setaf 1)$(tput setab 7)BUILD tests_clang_debug_cpp17 START$(tput sgr 0)\n"
mkdir build
cd build

PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
CC=clang CXX=clang++ \
cmake .. -DDEVELOPER_MODE=1 \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE \
			-DCXX_STANDARD=17

make -j2
test_command tests_clang_debug_cpp17 llvm

cd ..
rm -r build
printf "$(tput setaf 1)$(tput setab 7)BUILD tests_clang_debug_cpp17 END$(tput sgr 0)\n\n"

###############################################################################
# BUILD tests_gcc_debug
###############################################################################
printf "\n$(tput setaf 1)$(tput setab 7)BUILD tests_gcc_debug START$(tput sgr 0)\n"
mkdir build
cd build

PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
CC=gcc CXX=g++ \
cmake .. -DDEVELOPER_MODE=1 \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE

make -j2
test_command tests_gcc_debug

cd ..
rm -r build
printf "$(tput setaf 1)$(tput setab 7)BUILD tests_gcc_debug END$(tput sgr 0)\n\n"

###############################################################################
# BUILD tests_gcc_release_cpp17
###############################################################################
printf "\n$(tput setaf 1)$(tput setab 7)BUILD tests_gcc_release_cpp17 START$(tput sgr 0)\n"
mkdir build
cd build

PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
CC=gcc CXX=g++ \
cmake .. -DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE \
			-DCXX_STANDARD=17

make -j2
test_command tests_gcc_release_cpp17

cd ..
rm -r build
printf "$(tput setaf 1)$(tput setab 7)BUILD tests_gcc_release_cpp17 END$(tput sgr 0)\n\n"

###############################################################################
# BUILD tests_package
###############################################################################
printf "\n$(tput setaf 1)$(tput setab 7)BUILD tests_package START$(tput sgr 0)\n"
mkdir build
cd build

if [ $PACKAGE_MANAGER = "deb" ]; then
	echo $USERPASS | sudo -S dpkg -i /opt/pmdk-pkg/libpmem_*.deb /opt/pmdk-pkg/libpmem-dev_*.deb
	sudo dpkg -i /opt/pmdk-pkg/libpmemobj_*.deb /opt/pmdk-pkg/libpmemobj-dev_*.deb
elif [ $PACKAGE_MANAGER = "rpm" ]; then
	echo $USERPASS | sudo -S rpm -i /opt/pmdk-pkg/libpmem-*.rpm
	sudo rpm -i /opt/pmdk-pkg/libpmemobj-*.rpm
fi

cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
		-DCPACK_GENERATOR=$PACKAGE_MANAGER

make -j2
test_command tests_package

make package

# Make sure there is no libpmemobj++ currently installed
echo "---------------------------- Error expected! ------------------------------"
compile_example_standalone map_cli && exit 1
echo "---------------------------------------------------------------------------"

if [ $PACKAGE_MANAGER = "deb" ]; then
	sudo dpkg -i libpmemobj++*.deb
elif [ $PACKAGE_MANAGER = "rpm" ]; then
	sudo rpm -i libpmemobj++*.rpm
fi

cd ..
rm -rf build

# Verify installed package
compile_example_standalone map_cli

# Remove pkg-config and force cmake to use find_package while compiling example
if [ $PACKAGE_MANAGER = "deb" ]; then
	sudo dpkg -r --force-all pkg-config
elif [ $PACKAGE_MANAGER = "rpm" ]; then
	sudo rpm -e --nodeps pkgconf
fi

# Verify installed package using find_package
compile_example_standalone map_cli

printf "$(tput setaf 1)$(tput setab 7)BUILD tests_package END$(tput sgr 0)\n\n"

###############################################################################
# BUILD test findLIBPMEMOBJ.cmake
###############################################################################
printf "\n$(tput setaf 1)$(tput setab 7)BUILD tests_findLIBPMEMOBJ.cmake START$(tput sgr 0)\n"
mkdir build
cd build

CC=gcc CXX=g++ \
cmake .. -DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE \
			-DCXX_STANDARD=17

make -j2

cd ..
rm -r build
printf "$(tput setaf 1)$(tput setab 7)BUILD tests_findLIBPMEMOBJ.cmake END$(tput sgr 0)\n\n"

rm -r $INSTALL_DIR

# Trigger auto doc update on master
if [[ "$AUTO_DOC_UPDATE" == "1" ]]; then
	echo "Running auto doc update"

	mkdir doc_update
	cd doc_update

	$SCRIPTSDIR/run-doc-update.sh

	cd ..
	rm -rf doc_update
fi
