#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

#
# prepare-for-build.sh - prepares environment for the build
#		(when ./build.sh was used, it happens inside a Docker container)
#		and stores functions common to run-* scripts in this dir.
#

set -e

if [[ -z "${WORKDIR}" ]]; then
	echo "ERROR: The variable WORKDIR has to contain a path to the root " \
		"of this project - 'build' sub-directory will be created there."
	exit 1
fi

INSTALL_DIR=/tmp/libpmemobj-cpp
EXAMPLE_TEST_DIR=/tmp/build_example
TEST_DIR=${PMEMKV_TEST_DIR:-${DEFAULT_TEST_DIR}}

function sudo_password() {
	echo ${USERPASS} | sudo -Sk $*
}

echo "Current WORKDIR content:"
ls ${WORKDIR} -alh

if [ "${CI_RUN}" == "YES" ]; then
	echo "CI build: change WORKDIR's owner and prepare tmpfs device"
	sudo_password chown -R $(id -u).$(id -g) ${WORKDIR}

	sudo_password mkdir ${TEST_DIR}
	sudo_password chmod 0777 ${TEST_DIR}
	sudo_password mount -o size=2G -t tmpfs none ${TEST_DIR}
fi || true


## Helper functions, used in run-*.sh scripts
function workspace_cleanup() {
	echo "Cleanup build dirs"

	cd ${WORKDIR}
	rm -rf ${WORKDIR}/build
	rm -rf ${EXAMPLE_TEST_DIR}
	rm -rf ${INSTALL_DIR}
}

function upload_codecov() {
	printf "\n$(tput setaf 1)$(tput setab 7)COVERAGE ${FUNCNAME[0]} START$(tput sgr 0)\n"

	# set proper gcov command
	clang_used=$(cmake -LA -N . | grep CMAKE_CXX_COMPILER | grep clang | wc -c)
	if [[ ${clang_used} -gt 0 ]]; then
		gcovexe="llvm-cov gcov"
	else
		gcovexe="gcov"
	fi

	# run gcov exe, using their bash (remove parsed coverage files, set flag and exit 1 if not successful)
	# we rely on parsed report on codecov.io; the output is too long, hence it's disabled using -X flag
	/opt/scripts/codecov -c -F ${1} -Z -x "${gcovexe}" -X "gcovout"

	echo "Check for any leftover gcov files"
	leftover_files=$(find . -name "*.gcov")
	if [[ -n "${leftover_files}" ]]; then
		# display found files and exit with error (they all should be parsed)
		echo "${leftover_files}"
		return 1
	fi

	printf "$(tput setaf 1)$(tput setab 7)COVERAGE ${FUNCNAME[0]} END$(tput sgr 0)\n\n"
}

function compile_example_standalone() {
	example_name=${1}
	echo "Compile standalone example: ${example_name}"

	rm -rf ${EXAMPLE_TEST_DIR}
	mkdir ${EXAMPLE_TEST_DIR}
	cd ${EXAMPLE_TEST_DIR}

	cmake ${WORKDIR}/examples/${example_name}

	# exit on error
	if [[ $? != 0 ]]; then
		cd -
		return 1
	fi

	make -j$(nproc)
	cd -
}
