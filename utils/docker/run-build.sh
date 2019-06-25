#!/usr/bin/env bash
#
# Copyright 2016-2019, Intel Corporation
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

function upload_codecov() {
	clang_used=$(cmake -LA -N . | grep CMAKE_CXX_COMPILER | grep clang | wc -c)

	if [[ $clang_used > 0 ]]; then
		gcovexe="llvm-cov gcov"
	else
		gcovexe="gcov"
	fi

	# the output is redundant in this case, i.e. we rely on parsed report from codecov on github
	bash <(curl -s https://codecov.io/bash) -c -F $1 -x "$gcovexe" > /dev/null
	cleanup
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

function sudo_password() {
	echo $USERPASS | sudo -Sk $*
}

sudo_password mkdir /mnt/pmem
sudo_password chmod 0777 /mnt/pmem
sudo_password mount -o size=2G -t tmpfs none /mnt/pmem

cd $WORKDIR
INSTALL_DIR=/tmp/libpmemobj-cpp

mkdir $INSTALL_DIR

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
			-DCXX_STANDARD=17 \
			-DTESTS_USE_VALGRIND=0 \
			-DTEST_DIR=/mnt/pmem \
			-DTESTS_USE_FORCED_PMEM=1 \
			-DUSE_TBB=1 \

make -j2
ctest --output-on-failure --timeout 540
if [ "$COVERAGE" == "1" ]; then
	upload_codecov tests_clang_debug_cpp17
fi

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
			-DCOVERAGE=$COVERAGE \
			-DTESTS_USE_VALGRIND=1 \
			-DTEST_DIR=/mnt/pmem \
			-DTESTS_USE_FORCED_PMEM=1 \
			-DUSE_TBB=1 \

make -j2
if [ "$COVERAGE" == "1" ]; then
	# valgrind reports error when used with code coverage
	ctest -E "_memcheck|_drd|_helgrind|_pmemcheck" --timeout 540
	upload_codecov tests_gcc_debug
else
	PMREORDER_STACKTRACE_DEPTH=20 ctest --output-on-failure --timeout 540
fi

cd ..
rm -r build
printf "$(tput setaf 1)$(tput setab 7)BUILD tests_gcc_debug END$(tput sgr 0)\n\n"

###############################################################################
# BUILD tests_gcc_release_cpp17_no_valgrind
###############################################################################
printf "\n$(tput setaf 1)$(tput setab 7)BUILD tests_gcc_release_cpp17_no_valgrind START$(tput sgr 0)\n"
VALGRIND_PC_PATH=$(find /usr -name "valgrind.pc")
sudo_password mv $VALGRIND_PC_PATH tmp_valgrind_pc
mkdir build
cd build

PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
CC=gcc CXX=g++ \
cmake .. -DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
			-DTRACE_TESTS=1 \
			-DCOVERAGE=$COVERAGE \
			-DCXX_STANDARD=17 \
			-DTESTS_USE_VALGRIND=1 \
			-DTEST_DIR=/mnt/pmem \
			-DBUILD_EXAMPLES=0 \
			-DTESTS_USE_FORCED_PMEM=1

make -j2
ctest --output-on-failure --timeout 540
if [ "$COVERAGE" == "1" ]; then
	upload_codecov tests_gcc_release_cpp17_no_valgrind
fi

cd ..
rm -r build
sudo_password mv tmp_valgrind_pc $VALGRIND_PC_PATH
printf "$(tput setaf 1)$(tput setab 7)BUILD tests_gcc_release_cpp17_no_valgrind END$(tput sgr 0)\n\n"

###############################################################################
# BUILD tests_package
###############################################################################
printf "\n$(tput setaf 1)$(tput setab 7)BUILD tests_package START$(tput sgr 0)\n"
mkdir build
cd build

if [ $PACKAGE_MANAGER = "deb" ]; then
	sudo_password dpkg -i /opt/pmdk-pkg/libpmem_*.deb /opt/pmdk-pkg/libpmem-dev_*.deb
	sudo_password dpkg -i /opt/pmdk-pkg/libpmemobj_*.deb /opt/pmdk-pkg/libpmemobj-dev_*.deb
elif [ $PACKAGE_MANAGER = "rpm" ]; then
	sudo_password rpm -i /opt/pmdk-pkg/libpmem-*.rpm
	sudo_password rpm -i /opt/pmdk-pkg/libpmemobj-*.rpm
fi

CC=gcc CXX=g++ \
cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
		-DTESTS_USE_VALGRIND=0 \
		-DCPACK_GENERATOR=$PACKAGE_MANAGER

make -j2
ctest --output-on-failure --timeout 540

make package

# Make sure there is no libpmemobj++ currently installed
echo "---------------------------- Error expected! ------------------------------"
compile_example_standalone map_cli && exit 1
echo "---------------------------------------------------------------------------"

if [ $PACKAGE_MANAGER = "deb" ]; then
	sudo_password dpkg -i libpmemobj++*.deb
elif [ $PACKAGE_MANAGER = "rpm" ]; then
	sudo_password rpm -i libpmemobj++*.rpm
fi

cd ..
rm -rf build

# Verify installed package
compile_example_standalone map_cli

# Remove pkg-config and force cmake to use find_package while compiling example
if [ $PACKAGE_MANAGER = "deb" ]; then
	sudo_password dpkg -r --force-all pkg-config
elif [ $PACKAGE_MANAGER = "rpm" ]; then
	sudo_password rpm -e --nodeps pkgconf
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
