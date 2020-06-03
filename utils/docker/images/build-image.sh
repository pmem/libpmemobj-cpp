#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# build-image.sh <OS-OS_VER> - prepares a Docker image with <OS>-based
#                           environment for testing libpmemobj-cpp, according
#                           to the Dockerfile.<OS-OS_VER> file located
#                           in the same directory.
#
# The script can be run locally.
#

set -e

if [[ -z "$IMAGE_VER" ]]; then
	echo "WARN: IMAGE_VER environment variable is not set - a version of docker's image, " \
		 "usually related to project's release tag. It's used to tag the built docker image."
fi

OS__OS_VER=$1
TAG=${IMAGE_VER}-${OS__OS_VER}

function usage {
	echo "Usage:"
	echo "    build-image.sh <OS-OS_VER>"
	echo "where <OS-OS_VER>, for example, can be 'fedora-31', provided " \
		"a Dockerfile named 'Dockerfile.fedora-31' exists in the " \
		"current directory."
}

# Check if the argument is not empty
if [[ -z "$1" ]]; then
	usage
	exit 1
fi

# Check if the file Dockerfile.OS-OS_VER exists
if [[ ! -f "Dockerfile.$OS__OS_VER" ]]; then
	echo "Error: Dockerfile.$OS__OS_VER does not exist in current dir."
	echo
	usage
	exit 1
fi

# Build a Docker image tagged with ${DOCKERHUB_REPO}:${TAG}
docker build -t ${DOCKERHUB_REPO}:${TAG} \
	--build-arg http_proxy=$http_proxy \
	--build-arg https_proxy=$https_proxy \
	-f Dockerfile.${OS__OS_VER} .
