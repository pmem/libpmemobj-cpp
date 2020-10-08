#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2017-2020, Intel Corporation

#
# build-local.sh - runs a Docker container from a Docker image with environment
#                  prepared for running libpmemobj-cpp builds and tests.
#
# Notes:
# - run this script from its location or set the variable 'HOST_WORKDIR' to
#   where the root of this project is on the host machine,
# - set variables 'OS' and 'OS_VER' properly to a system you want to build this
#	repo on (for proper values take a look at the list of Dockerfiles at the
#   utils/docker/images directory), e.g. OS=ubuntu, OS_VER=19.10.
#

set -e

source $(dirname $0)/set-ci-vars.sh
source $(dirname $0)/set-vars.sh

doc_variables_error="To build documentation and upload it as a Github pull request, \
variables 'DOC_UPDATE_BOT_NAME', 'DOC_REPO_OWNER' and 'DOC_UPDATE_GITHUB_TOKEN' have to be provided. \
For more details please read CONTRIBUTING.md"

if [[ -z "$OS" || -z "$OS_VER" ]]; then
	echo "ERROR: The variables OS and OS_VER have to be set " \
		"(eg. OS=fedora, OS_VER=31)."
	exit 1
fi

if [[ -z "$HOST_WORKDIR" ]]; then
	echo "ERROR: The variable HOST_WORKDIR has to contain a path to " \
		"the root of this project on the host machine"
	exit 1
fi

imageName=${DOCKERHUB_REPO}:1.12-${OS}-${OS_VER}
containerName=libpmemobj-cpp-${OS}-${OS_VER}

if [[ "$command" == "" ]]; then
	case $TYPE in
	debug)
		builds=(tests_gcc_debug_cpp14_no_valgrind
				tests_clang_debug_cpp17_no_valgrind)
		command="./run-build.sh ${builds[@]}";
		;;
	release)
		builds=(tests_gcc_release_cpp17_no_valgrind
				tests_clang_release_cpp11_no_valgrind)
		command="./run-build.sh ${builds[@]}";
		;;
	valgrind)
		builds=(tests_gcc_debug_cpp14_valgrind_other)
		command="./run-build.sh ${builds[@]}";
		;;
	memcheck_drd)
		builds=(tests_gcc_debug_cpp14_valgrind_memcheck_drd)
		command="./run-build.sh ${builds[@]}";
		;;
	package)
		builds=(tests_package
			tests_findLIBPMEMOBJ_cmake)
		command="./run-build.sh ${builds[@]}";
		;;
	coverity)
		command="./run-coverity.sh";
		;;
	doc)
		if [[ -z "${DOC_UPDATE_BOT_NAME}" || -z "${DOC_UPDATE_GITHUB_TOKEN}" || -z "${DOC_REPO_OWNER}" ]]; then
			echo "${doc_variables_error}"
			exit 0
		fi
		command="./run-doc-update.sh";
		;;
	*)
		echo "ERROR: wrong build TYPE"
		exit 1
		;;
	esac
fi

if [ "$COVERAGE" == "1" ]; then
	docker_opts="${docker_opts} `bash <(curl -s https://codecov.io/env)`";
fi

if [ -n "$DNS_SERVER" ]; then DNS_SETTING=" --dns=$DNS_SERVER "; fi

# Check if we are running on a CI (Travis or GitHub Actions)
[ -n "$GITHUB_ACTIONS" -o -n "$TRAVIS" ] && CI_RUN="YES" || CI_RUN="NO"

# do not allocate a pseudo-TTY if we are running on GitHub Actions
[ ! $GITHUB_ACTIONS ] && TTY='-t' || TTY=''

WORKDIR=/libpmemobj-cpp
SCRIPTSDIR=$WORKDIR/utils/docker

echo Building on ${OS}-${OS_VER}

# Run a container with
#  - environment variables set (--env)
#  - host directory containing source mounted (-v)
#  - working directory set (-w)
docker run --privileged=true --name=$containerName -i $TTY \
	$DNS_SETTING \
	${docker_opts} \
	--env http_proxy=$http_proxy \
	--env https_proxy=$https_proxy \
	--env WORKDIR=$WORKDIR \
	--env SCRIPTSDIR=$SCRIPTSDIR \
	--env COVERAGE=$COVERAGE \
	--env CI_RUN=$CI_RUN \
	--env TRAVIS=$TRAVIS \
	--env GITHUB_REPO=$GITHUB_REPO \
	--env CI_COMMIT_RANGE=$CI_COMMIT_RANGE \
	--env CI_COMMIT=$CI_COMMIT \
	--env CI_REPO_SLUG=$CI_REPO_SLUG \
	--env CI_BRANCH=$CI_BRANCH \
	--env CI_EVENT_TYPE=$CI_EVENT_TYPE \
	--env GITHUB_ACTIONS=$GITHUB_ACTIONS \
	--env GITHUB_HEAD_REF=$GITHUB_HEAD_REF \
	--env GITHUB_REPO=$GITHUB_REPO \
	--env GITHUB_REPOSITORY=$GITHUB_REPOSITORY \
	--env GITHUB_REF=$GITHUB_REF \
	--env GITHUB_RUN_ID=$GITHUB_RUN_ID \
	--env GITHUB_SHA=$GITHUB_SHA \
	--env DOC_UPDATE_GITHUB_TOKEN=$DOC_UPDATE_GITHUB_TOKEN \
	--env DOC_UPDATE_BOT_NAME=$DOC_UPDATE_BOT_NAME \
	--env DOC_REPO_OWNER=$DOC_REPO_OWNER \
	--env COVERITY_SCAN_TOKEN=$COVERITY_SCAN_TOKEN \
	--env COVERITY_SCAN_NOTIFICATION_EMAIL=$COVERITY_SCAN_NOTIFICATION_EMAIL \
	--env CHECK_CPP_STYLE=${CHECK_CPP_STYLE:-OFF} \
	--env TESTS_LONG=${TESTS_LONG:-OFF} \
	--env TESTS_TBB=${TESTS_TBB:-ON} \
	--env TESTS_PMREORDER=${TESTS_PMREORDER:-ON} \
	--env TZ='Europe/Warsaw' \
	--shm-size=4G \
	-v $HOST_WORKDIR:$WORKDIR \
	-v /etc/localtime:/etc/localtime \
	-w $SCRIPTSDIR \
	$imageName $command
