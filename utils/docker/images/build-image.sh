#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# build-image.sh <OS-VER> - prepares a Docker image with <OS>-based
#                           environment for testing libpmemobj-cpp, according
#                           to the Dockerfile.<OS-VER> file located
#                           in the same directory.
#
# The script can be run locally.
#

set -e

function usage {
	echo
	echo "Usage:"
	echo "    build-image.sh <OS-VER>"
	echo "where <OS-VER>, for example, can be 'fedora-31', provided " \
		"a Dockerfile named 'Dockerfile.fedora-31' exists in the " \
		"current directory."
}
OS__OS_VER=${1}

echo "Check if the argument is not empty"
if [[ -z "${OS__OS_VER}" ]]; then
	usage
	exit 1
fi

if [[ -z "${CONTAINER_REG}" ]]; then
	echo "CONTAINER_REG environment variable is not set"
	exit 1
fi

echo "Check if the file Dockerfile.${OS__OS_VER} exists"
if [[ ! -f "Dockerfile.${OS__OS_VER}" ]]; then
	echo "Error: Dockerfile.${OS__OS_VER} does not exist."
	usage
	exit 1
fi

echo "Build a Docker image tagged with ${CONTAINER_REG}:1.12-${OS__OS_VER}"
docker build -t ${CONTAINER_REG}:1.12-${OS__OS_VER} \
	--build-arg http_proxy=$http_proxy \
	--build-arg https_proxy=$https_proxy \
	-f Dockerfile.${OS__OS_VER} .
