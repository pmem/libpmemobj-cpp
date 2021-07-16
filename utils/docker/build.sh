#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2021, Intel Corporation

#
# build.sh - runs a Docker container from a Docker image with environment
#		prepared for running libpmemobj-cpp builds and tests. It uses Docker image
#		tagged as described in ./images/build-image.sh.
#
# Notes:
# - set env var 'HOST_WORKDIR' to where the root of this project is on the host machine,
# - set env var 'OS' and 'OS_VER' properly to a system/Docker you want to build this
#	repo on (for proper values take a look at the list of Dockerfiles at the
#	utils/docker/images directory in this repo), e.g. OS=ubuntu, OS_VER=20.04,
# - set env var 'CONTAINER_REG' to container registry address
#	[and possibly user/org name, and package name], e.g. "<CR_addr>/pmem/libpmemobj-cpp",
# - set env var 'DNS_SERVER' if you use one,
# - set env var 'COMMAND' to execute specific command within Docker container or
#	env var 'TYPE' to pick command based on one of the predefined types of build (see below).
#

set -e

source $(dirname ${0})/set-ci-vars.sh
IMG_VER=${IMG_VER:-devel}
TAG="${OS}-${OS_VER}-${IMG_VER}"
IMAGE_NAME=${CONTAINER_REG}:${TAG}
CONTAINER_NAME=libpmemobj-cpp-${OS}-${OS_VER}
WORKDIR=/libpmemobj-cpp  # working dir within Docker container
SCRIPTSDIR=${WORKDIR}/utils/docker

if [[ -z "${OS}" || -z "${OS_VER}" ]]; then
	echo "ERROR: The variables OS and OS_VER have to be set " \
		"(e.g. OS=fedora, OS_VER=32)."
	exit 1
fi

if [[ -z "${HOST_WORKDIR}" ]]; then
	echo "ERROR: The variable HOST_WORKDIR has to contain a path to " \
		"the root of this project on the host machine."
	exit 1
fi

if [[ -z "${CONTAINER_REG}" ]]; then
	echo "ERROR: CONTAINER_REG environment variable is not set " \
		"(e.g. \"<registry_addr>/<org_name>/<package_name>\")."
	exit 1
fi

# Set command to execute in the Docker container
if [[ -z "${COMMAND}" ]]; then
	echo "COMMAND will be based on the type of build: ${TYPE}"
	case ${TYPE} in
	debug)
		builds=(tests_gcc_debug_cpp14_no_valgrind
				tests_clang_debug_cpp17_no_valgrind)
		COMMAND="./run-build.sh ${builds[@]}";
		;;
	release)
		builds=(tests_gcc_release_cpp17_no_valgrind
				tests_clang_release_cpp11_no_valgrind
				tests_clang_release_cpp20_no_valgrind)
		COMMAND="./run-build.sh ${builds[@]}";
		;;
	valgrind)
		builds=(tests_gcc_debug_cpp14_valgrind_other)
		COMMAND="./run-build.sh ${builds[@]}";
		;;
	memcheck_drd)
		builds=(tests_gcc_debug_cpp14_valgrind_memcheck_drd)
		COMMAND="./run-build.sh ${builds[@]}";
		;;
	package)
		builds=(tests_package
			tests_findLIBPMEMOBJ_cmake
			tests_cmake)
		COMMAND="./run-build.sh ${builds[@]}";
		;;
	coverity)
		COMMAND="./run-coverity.sh";
		;;
	doc)
		COMMAND="./run-doc-update.sh";
		;;
	*)
		echo "ERROR: wrong build TYPE"
		exit 1
		;;
	esac
fi
echo "COMMAND to execute within Docker container: ${COMMAND}"

if [ "${COVERAGE}" == "1" ]; then
	DOCKER_OPTS="${DOCKER_OPTS} $(bash <(curl -s https://codecov.io/env))";
fi

if [ -n "${DNS_SERVER}" ]; then DOCKER_OPTS="${DOCKER_OPTS} --dns=${DNS_SERVER}"; fi

# Check if we are running on a CI (Travis or GitHub Actions)
[ -n "${GITHUB_ACTIONS}" -o -n "${TRAVIS}" ] && CI_RUN="YES" || CI_RUN="NO"

# Do not allocate a pseudo-TTY if we are running on GitHub Actions
[ ! "${GITHUB_ACTIONS}" ] && DOCKER_OPTS="${DOCKER_OPTS} --tty=true"


echo "Running build using Docker image: ${IMAGE_NAME}"

# Run a container with
#  - environment variables set (--env)
#  - host directory containing source mounted (-v)
#  - working directory set (-w)
docker run --privileged=true --name=${CONTAINER_NAME} -i \
	${DOCKER_OPTS} \
	--env http_proxy=${http_proxy} \
	--env https_proxy=${https_proxy} \
	--env TERM=xterm-256color \
	--env WORKDIR=${WORKDIR} \
	--env SCRIPTSDIR=${SCRIPTSDIR} \
	--env GITHUB_REPO=${GITHUB_REPO} \
	--env CI_RUN=${CI_RUN} \
	--env TRAVIS=${TRAVIS} \
	--env GITHUB_ACTIONS=${GITHUB_ACTIONS} \
	--env CI_COMMIT=${CI_COMMIT} \
	--env CI_COMMIT_RANGE=${CI_COMMIT_RANGE} \
	--env CI_BRANCH=${CI_BRANCH} \
	--env CI_EVENT_TYPE=${CI_EVENT_TYPE} \
	--env CI_REPO_SLUG=${CI_REPO_SLUG} \
	--env DOC_UPDATE_GITHUB_TOKEN=${DOC_UPDATE_GITHUB_TOKEN} \
	--env DOC_UPDATE_BOT_NAME=${DOC_UPDATE_BOT_NAME} \
	--env DOC_REPO_OWNER=${DOC_REPO_OWNER} \
	--env COVERITY_SCAN_TOKEN=${COVERITY_SCAN_TOKEN} \
	--env COVERITY_SCAN_NOTIFICATION_EMAIL=${COVERITY_SCAN_NOTIFICATION_EMAIL} \
	--env CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-OFF} \
	--env COVERAGE=${COVERAGE} \
	--env DEFAULT_TEST_DIR=/mnt/pmem \
	--env TESTS_LONG=${TESTS_LONG:-OFF} \
	--env TESTS_TBB=${TESTS_TBB:-ON} \
	--env TESTS_PMREORDER=${TESTS_PMREORDER:-ON} \
	--env TESTS_PACKAGES=${TESTS_PACKAGES:-ON} \
	--env TESTS_USAN=${TESTS_USAN:-OFF} \
	--env TESTS_UBSAN=${TESTS_UBSAN:-OFF} \
	--env TEST_TIMEOUT=${TEST_TIMEOUT} \
	--env TZ='Europe/Warsaw' \
	--shm-size=4G \
	-v ${HOST_WORKDIR}:${WORKDIR} \
	-v /etc/localtime:/etc/localtime \
	-w ${SCRIPTSDIR} \
	${IMAGE_NAME} ${COMMAND}
