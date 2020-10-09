#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# push-image.sh - pushes the Docker image tagged with OS-VER to the ${DOCKERHUB_REPO}.
#
# The script utilizes ${GH_CONTAINER_REG_USER} and ${DOCKERHUB_PASSWORD} variables to
# log in to the ${DOCKERHUB_REPO}. The variables can be set in the CI's configuration
# for automated builds.
#

set -e

if [[ -z "${OS}" ]]; then
	echo "OS environment variable is not set"
	exit 1
fi

if [[ -z "${OS_VER}" ]]; then
	echo "OS_VER environment variable is not set"
	exit 1
fi

if [[ -z "${DOCKERHUB_REPO}" ]]; then
	echo "DOCKERHUB_REPO environment variable is not set"
	exit 1
fi

TAG="1.12-${OS}-${OS_VER}"

echo "Check if the image tagged with ${DOCKERHUB_REPO}:${TAG} exists locally"
if [[ ! $(docker images -a | awk -v pattern="^${DOCKERHUB_REPO}:${TAG}\$" \
	'$1":"$2 ~ pattern') ]]
then
	echo "ERROR: Docker image tagged ${DOCKERHUB_REPO}:${TAG} does not exist locally."
	exit 1
fi

echo "Log in to the repository"
echo "${GH_CONTAINER_REG_PAT}" | docker login ghcr.io -u="${GH_CONTAINER_REG_USER}" --password-stdin

echo "Push the image to the repository"
docker push ${DOCKERHUB_REPO}:${TAG}
