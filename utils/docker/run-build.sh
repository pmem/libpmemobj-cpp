#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2021, Intel Corporation

#
# run-build.sh [build_step]...
#		in CI it's run inside a Docker container, called by ./build.sh .
#		It can be also run locally (but with caution, it may affect local environment).
#		Executes libpmemobj-cpp builds (given as params; defined here as functions).
#

set -e

source $(dirname ${0})/prepare-for-build.sh

# params set for this file; if not previously set, the right-hand param is used
TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}
CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-ON}
TESTS_LONG=${TESTS_LONG:-OFF}
TESTS_TBB=${TESTS_TBB:-ON}
TESTS_PMREORDER=${TESTS_PMREORDER:-ON}
TESTS_PACKAGES=${TESTS_PACKAGES:-ON}
TESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM:-ON}
TEST_TIMEOUT=${TEST_TIMEOUT:-600}

export PMREORDER_STACKTRACE_DEPTH=20

###############################################################################
# BUILD tests_clang_debug_cpp17_no_valgrind llvm
###############################################################################
function tests_clang_debug_cpp17_no_valgrind() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/pmdk/lib/pkgconfig/ \
	CC=clang CXX=clang++ \
	cmake .. -DDEVELOPER_MODE=1 \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
		-DTRACE_TESTS=1 \
		-DCOVERAGE=${COVERAGE} \
		-DCXX_STANDARD=17 \
		-DTESTS_USE_VALGRIND=0 \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_TBB=${TESTS_TBB} \
		-DTESTS_PMREORDER=${TESTS_PMREORDER} \
		-DTEST_DIR=${TEST_DIR} \
		-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM} \
		-DTESTS_COMPATIBILITY=1

	make -j$(nproc)
	ctest --output-on-failure -E "_pmreorder" --timeout ${TEST_TIMEOUT}
	if [ "${COVERAGE}" == "1" ]; then
		upload_codecov tests_clang_debug_cpp17
	fi

	workspace_cleanup
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_clang_release_cpp11_no_valgrind llvm
###############################################################################
function tests_clang_release_cpp11_no_valgrind() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build
	cd build

	PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/pmdk/lib/pkgconfig/ \
	CC=clang CXX=clang++ \
	cmake .. -DDEVELOPER_MODE=1 \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
		-DTRACE_TESTS=1 \
		-DCOVERAGE=${COVERAGE} \
		-DCXX_STANDARD=11 \
		-DTESTS_USE_VALGRIND=0 \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_PMREORDER=${TESTS_PMREORDER} \
		-DTEST_DIR=${TEST_DIR} \
		-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM} \
		-DTESTS_COMPATIBILITY=1

	make -j$(nproc)
	ctest --output-on-failure -E "_pmreorder" --timeout ${TEST_TIMEOUT}
	if [ "${COVERAGE}" == "1" ]; then
		upload_codecov tests_clang_release_cpp11
	fi

	workspace_cleanup
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD build_gcc_debug_cpp14 (no tests)
###############################################################################
function build_gcc_debug_cpp14() {
	mkdir build
	cd build

	PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/pmdk/lib/pkgconfig/ \
	CC=gcc CXX=g++ \
	cmake .. -DDEVELOPER_MODE=1 \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DCMAKE_BUILD_TYPE=Debug \
		-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
		-DTRACE_TESTS=1 \
		-DCOVERAGE=${COVERAGE} \
		-DCXX_STANDARD=14 \
		-DTESTS_USE_VALGRIND=1 \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_TBB=${TESTS_TBB} \
		-DTESTS_PMREORDER=${TESTS_PMREORDER} \
		-DTEST_DIR=${TEST_DIR} \
		-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM} \
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
	ctest -E "_memcheck|_drd|_helgrind|_pmemcheck|_pmreorder" --timeout ${TEST_TIMEOUT} --output-on-failure
	if [ "${COVERAGE}" == "1" ]; then
		upload_codecov tests_gcc_debug
	fi
	workspace_cleanup
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_debug_cpp14_valgrind_memcheck_drd
###############################################################################
function tests_gcc_debug_cpp14_valgrind_memcheck_drd() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	build_gcc_debug_cpp14
	ctest -R "_memcheck|_drd" --timeout ${TEST_TIMEOUT} --output-on-failure
	workspace_cleanup
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_debug_cpp14_valgrind_other
###############################################################################
function tests_gcc_debug_cpp14_valgrind_other() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	build_gcc_debug_cpp14
	ctest -E "_none|_memcheck|_drd" --timeout ${TEST_TIMEOUT} --output-on-failure
	ctest -R "_pmreorder" --timeout ${TEST_TIMEOUT} --output-on-failure
	workspace_cleanup
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_gcc_release_cpp17_no_valgrind
###############################################################################
function tests_gcc_release_cpp17_no_valgrind() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"
	mkdir build && cd build

	PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/pmdk/lib/pkgconfig/ \
	CC=gcc CXX=g++ \
	cmake .. -DDEVELOPER_MODE=1 \
		-DCHECK_CPP_STYLE=${CHECK_CPP_STYLE} \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
		-DTRACE_TESTS=1 \
		-DCOVERAGE=${COVERAGE} \
		-DCXX_STANDARD=17 \
		-DTESTS_USE_VALGRIND=0 \
		-DTESTS_LONG=${TESTS_LONG} \
		-DTESTS_TBB=${TESTS_TBB} \
		-DTESTS_PMREORDER=${TESTS_PMREORDER} \
		-DTEST_DIR=${TEST_DIR} \
		-DBUILD_EXAMPLES=0 \
		-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM}

	make -j$(nproc)
	ctest --output-on-failure --timeout ${TEST_TIMEOUT}
	if [ "${COVERAGE}" == "1" ]; then
		upload_codecov tests_gcc_release_cpp17_no_valgrind
	fi

	workspace_cleanup
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_package
###############################################################################
function tests_package() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"

	[ ! "${TESTS_PACKAGES}" = "ON" ] && echo "Skipping testing packages, TESTS_PACKAGES variable is not set."

	if ! ls /opt/pmdk-pkg/libpmem* > /dev/null 2>&1; then
		echo "ERROR: There are no PMDK packages in /opt/pmdk-pkg - they are required for package test(s)."
		printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
		return 1
	fi

	mkdir build
	cd build

	if [ ${PACKAGE_MANAGER} = "deb" ]; then
		sudo_password dpkg -i /opt/pmdk-pkg/libpmem_*.deb /opt/pmdk-pkg/libpmem-dev_*.deb
		sudo_password dpkg -i /opt/pmdk-pkg/libpmemobj_*.deb /opt/pmdk-pkg/libpmemobj-dev_*.deb
	elif [ ${PACKAGE_MANAGER} = "rpm" ]; then
		sudo_password rpm -i /opt/pmdk-pkg/libpmem*.rpm /opt/pmdk-pkg/pmdk-debuginfo-*.rpm
	else
		echo "ERROR: skipping building of packages because PACKAGE_MANAGER is not equal to 'rpm' nor 'deb' ..."
		return 1
	fi

	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_INSTALL_PREFIX=/usr \
		-DTESTS_USE_VALGRIND=0 \
		-DTESTS_LONG=OFF \
		-DTESTS_TBB=OFF \
		-DTESTS_PMREORDER=OFF \
		-DBUILD_EXAMPLES=0 \
		-DCPACK_GENERATOR=${PACKAGE_MANAGER} \
		-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM}

	make -j$(nproc)
	ctest --output-on-failure --timeout ${TEST_TIMEOUT}

	make -j$(nproc) package

	echo "Make sure there is no libpmemobj++ currently installed."
	echo "---------------------------- Error expected! ------------------------------"
	compile_example_standalone map_cli && exit 1
	echo "---------------------------------------------------------------------------"

	if [ ${PACKAGE_MANAGER} = "deb" ]; then
		sudo_password dpkg -i libpmemobj++*.deb
	elif [ ${PACKAGE_MANAGER} = "rpm" ]; then
		sudo_password rpm -i libpmemobj++*.rpm
	fi

	workspace_cleanup

	echo "Verify installed package."
	compile_example_standalone map_cli

	# Remove pkg-config and force cmake to use find_package while compiling example
	if [ ${PACKAGE_MANAGER} = "deb" ]; then
		sudo_password dpkg -r --force-all pkg-config
	elif [ ${PACKAGE_MANAGER} = "rpm" ]; then
		# most rpm based OSes use the 'pkgconf' name, only openSUSE uses 'pkg-config'
		sudo_password rpm -e --nodeps pkgconf || sudo_password rpm -e --nodeps pkg-config
	fi

	echo "Verify installed package using find_package."
	compile_example_standalone map_cli

	workspace_cleanup
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
		-DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
		-DTESTS_USE_VALGRIND=OFF \
		-DTESTS_TBB=OFF \
		-DTESTS_LONG=OFF \
		-DTRACE_TESTS=1 \
		-DBUILD_EXAMPLES=0 \
		-DCOVERAGE=${COVERAGE} \
		-DCXX_STANDARD=17 \
		-DTESTS_USE_FORCED_PMEM=${TESTS_USE_FORCED_PMEM}

	make -j$(nproc)

	workspace_cleanup
	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

###############################################################################
# BUILD tests_cmake (tests for the build system itself)
###############################################################################
function tests_cmake() {
	printf "\n$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} START$(tput sgr 0)\n"

	echo "Rename valgrind.pc and build with TESTS_USE_VALGRIND=1 CMake option." \
		"Build should fail, because Valgrind couldn't be found."
	VALGRIND_PC_PATH=$(find /usr -name "valgrind.pc" 2>/dev/null || true)
	[ -z "${VALGRIND_PC_PATH}" ] && echo "Error: cannot find 'valgrind.pc' file" && exit 1
	echo "valgrind.pc found in: ${VALGRIND_PC_PATH}"
	sudo_password mv ${VALGRIND_PC_PATH} tmp_valgrind_pc

	mkdir build && cd build

	echo "---------------------------- Error expected! ------------------------------"
	PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/opt/pmdk/lib/pkgconfig/ \
	CC=gcc CXX=g++ \
	cmake .. -DCMAKE_BUILD_TYPE=Debug \
		-DTEST_DIR=${TEST_DIR} \
		-DTESTS_USE_VALGRIND=1 \
	&& exit 1
	echo "----------------------------------------------------------------------------"

	workspace_cleanup
	echo "Recover valgrind.pc"
	sudo_password mv tmp_valgrind_pc ${VALGRIND_PC_PATH}

	printf "$(tput setaf 1)$(tput setab 7)BUILD ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}


# Main
echo "### run-build.sh starts here ###"
workspace_cleanup

echo "Run build steps passed as script arguments"
build_steps=$@
echo "Defined build steps: ${build_steps}"

if [[ -z "${build_steps}" ]]; then
	echo "ERROR: The variable build_steps with selected builds to run is not set!"
	exit 1
fi

for build in ${build_steps}
do
	${build}
done
