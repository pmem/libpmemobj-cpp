#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# run-build.sh - is called inside a Docker container; prepares the environment
#                and starts builds of libpmemobj-cpp.
#

set -e

source `dirname $0`/prepare-for-build.sh

CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-ON}
TESTS_LONG=${TESTS_LONG:-OFF}
TESTS_TBB=${TESTS_TBB:-ON}
INSTALL_DIR=/tmp/libpmemobj-cpp

export PMREORDER_STACKTRACE_DEPTH=20

function upload_codecov() {
	printf "\n$(tput setaf 1)$(tput setab 7)COVERAGE ${FUNCNAME[0]} START$(tput sgr 0)\n"

	# set proper gcov command
	clang_used=$(cmake -LA -N . | grep CMAKE_CXX_COMPILER | grep clang | wc -c)
	if [[ $clang_used > 0 ]]; then
		gcovexe="llvm-cov gcov"
	else
		gcovexe="gcov"
	fi

	# run gcov exe, using their bash (remove parsed coverage files, set flag and exit 1 if not successful)
	# we rely on parsed report on codecov.io; the output is too long, hence it's disabled using -X flag
	/opt/scripts/codecov -c -F $1 -Z -x "$gcovexe" -X "gcovout"

	printf "check for any leftover gcov files\n"
	leftover_files=$(find . -name "*.gcov")
	if [[ -n "$leftover_files" ]]; then
		# display found files and exit with error (they all should be parsed)
		echo "$leftover_files"
		return 1
	fi

	printf "$(tput setaf 1)$(tput setab 7)COVERAGE ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
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

	make -j$(nproc)
	cd -
}

function sudo_password() {
	echo $USERPASS | sudo -Sk $*
}

###############################################################################
# BUILD tests_clang_debug_cpp17_no_valgrind llvm
###############################################################################
function tests_clang_debug_cpp17_no_valgrind() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
	CC=clang CXX=clang++ \
	cmake .. -DDEVELOPER_MODE=1 \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
		-DTRACE_TESTS=1 \
		-DCOVERAGE=$COVERAGE \
		-DCXX_STANDARD=17 \
		-DTESTS_USE_VALGRIND=0 \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_TBB=${TESTS_TBB} \
		-DTEST_DIR=/mnt/pmem \
		-DTESTS_USE_FORCED_PMEM=1 \
		-DTESTS_COMPATIBILITY=1 \
		-DTESTS_CONCURRENT_GDB=1

	make -j$(nproc)
	ctest --output-on-failure -E "_pmreorder" --timeout 590
	if [ "$COVERAGE" == "1" ]; then
		upload_codecov tests_clang_debug_cpp17
	fi

	cd ..
	rm -rf build
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_clang_release_cpp11_no_valgrind llvm
###############################################################################
function tests_clang_release_cpp11_no_valgrind() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
	CC=clang CXX=clang++ \
	cmake .. -DDEVELOPER_MODE=1 \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
		-DTRACE_TESTS=1 \
		-DCOVERAGE=$COVERAGE \
		-DCXX_STANDARD=11 \
		-DTESTS_USE_VALGRIND=0 \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTEST_DIR=/mnt/pmem \
		-DTESTS_USE_FORCED_PMEM=1 \
		-DTESTS_COMPATIBILITY=1

	make -j$(nproc)
	ctest --output-on-failure -E "_pmreorder"  --timeout 540
	if [ "$COVERAGE" == "1" ]; then
		upload_codecov tests_clang_release_cpp11
	fi

	cd ..
	rm -rf build
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD build_gcc_debug_cpp14 (no tests)
###############################################################################
function build_gcc_debug_cpp14() {
	mkdir build
	cd build

	PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
	CC=gcc CXX=g++ \
	cmake .. -DDEVELOPER_MODE=1 \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
		-DTRACE_TESTS=1 \
		-DCOVERAGE=$COVERAGE \
		-DCXX_STANDARD=14 \
		-DTESTS_USE_VALGRIND=1 \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_TBB=${TESTS_TBB} \
		-DTEST_DIR=/mnt/pmem \
		-DTESTS_USE_FORCED_PMEM=1 \
		-DTESTS_CONCURRENT_HASH_MAP_DRD_HELGRIND=1 \
		-DTESTS_COMPATIBILITY=1 \
		-DTESTS_CONCURRENT_GDB=1

	make -j$(nproc)
}

###############################################################################
# BUILD tests_gcc_debug_cpp14_no_valgrind
###############################################################################
function tests_gcc_debug_cpp14_no_valgrind() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	build_gcc_debug_cpp14
	ctest -E "_memcheck|_drd|_helgrind|_pmemcheck|_pmreorder" --timeout 590 --output-on-failure
	if [ "$COVERAGE" == "1" ]; then
		upload_codecov tests_gcc_debug
	fi
	cd ..
	rm -rf build
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_debug_cpp14_valgrind_memcheck_drd
###############################################################################
function tests_gcc_debug_cpp14_valgrind_memcheck_drd() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	build_gcc_debug_cpp14
	ctest -R "_memcheck|_drd" --timeout 590 --output-on-failure
	cd ..
	rm -rf build
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_debug_cpp14_valgrind_other
###############################################################################
function tests_gcc_debug_cpp14_valgrind_other() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	build_gcc_debug_cpp14
	ctest -E "_none|_memcheck|_drd" --timeout 590 --output-on-failure
	ctest -R "_pmreorder" --timeout 590 --output-on-failure
	cd ..
	rm -rf build
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_release_cpp17_no_valgrind
###############################################################################
function tests_gcc_release_cpp17_no_valgrind() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	# It is a test of the build system: move valgrind to other location and build with
	# TESTS_USE_VALGRIND=1. Expected behaviour is to get tests with suffix
	# _SKIPPED_BECAUSE_OF_MISSING_VALGRIND
	VALGRIND_PC_PATH=$(find /usr -name "valgrind.pc" 2>/dev/null || true)
	[ "$VALGRIND_PC_PATH" == "" ] && echo "Error: cannot find 'valgrind.pc' file" && exit 1
	sudo_password mv $VALGRIND_PC_PATH tmp_valgrind_pc
	mkdir build
	cd build

	PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig/ \
	CC=gcc CXX=g++ \
	cmake .. -DDEVELOPER_MODE=1 \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
		-DTRACE_TESTS=1 \
		-DCOVERAGE=$COVERAGE \
		-DCXX_STANDARD=17 \
		-DTESTS_USE_VALGRIND=1 \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_TBB=${TESTS_TBB} \
		-DTEST_DIR=/mnt/pmem \
		-DBUILD_EXAMPLES=0 \
		-DTESTS_USE_FORCED_PMEM=1

	make -j$(nproc)
	ctest --output-on-failure --timeout 590
	if [ "$COVERAGE" == "1" ]; then
		upload_codecov tests_gcc_release_cpp17_no_valgrind
	fi

	cd ..
	rm -r build
	#Recover valgrind
	sudo_password mv tmp_valgrind_pc $VALGRIND_PC_PATH
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_package
###############################################################################
function tests_package() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	if [ $PACKAGE_MANAGER = "deb" ]; then
		sudo_password dpkg -i /opt/pmdk-pkg/libpmem_*.deb /opt/pmdk-pkg/libpmem-dev_*.deb
		sudo_password dpkg -i /opt/pmdk-pkg/libpmemobj_*.deb /opt/pmdk-pkg/libpmemobj-dev_*.deb
	elif [ $PACKAGE_MANAGER = "rpm" ]; then
		sudo_password rpm -i /opt/pmdk-pkg/libpmem*.rpm /opt/pmdk-pkg/pmdk-debuginfo-*.rpm
	fi

	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
		-DTESTS_USE_VALGRIND=0 \
		-DTESTS_LONG=OFF \
		-DTESTS_TBB=OFF \
		-DBUILD_EXAMPLES=0 \
		-DCPACK_GENERATOR=$PACKAGE_MANAGER \
		-DTESTS_USE_FORCED_PMEM=1

	make -j$(nproc)
	ctest --output-on-failure --timeout 590

	make -j$(nproc) package

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
		# most rpm based OSes use the 'pkgconf' name, only openSUSE uses 'pkg-config'
		sudo_password rpm -e --nodeps pkgconf || sudo_password rpm -e --nodeps pkg-config
	fi

	# Verify installed package using find_package
	compile_example_standalone map_cli

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_findLIBPMEMOBJ.cmake (pkg-config not set, try to find libpmemobj "manually")
###############################################################################
function tests_findLIBPMEMOBJ_cmake()
{
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
		-DTESTS_TBB=OFF \
		-DTESTS_LONG=OFF \
		-DTRACE_TESTS=1 \
		-DBUILD_EXAMPLES=0 \
		-DCOVERAGE=$COVERAGE \
		-DCXX_STANDARD=17 \
		-DTESTS_USE_FORCED_PMEM=1

	make -j$(nproc)

	cd ..
	rm -r build
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

# Main:
sudo_password mkdir /mnt/pmem
sudo_password chmod 0777 /mnt/pmem
sudo_password mount -o size=2G -t tmpfs none /mnt/pmem
mkdir $INSTALL_DIR

cd $WORKDIR

# Run all build steps passed as script arguments
build_steps=$@
for build in $build_steps
do
	$build
done

rm -r $INSTALL_DIR
