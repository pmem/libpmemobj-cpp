#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2016-2020, Intel Corporation

#
# pull-or-rebuild-image.sh - rebuilds the Docker image used in the
#                            current build if necessary.
#
# The script rebuilds the Docker image if:
# 1. the Dockerfile for the current OS version (Dockerfile.${OS}-${OS_VER})
#    or any .sh script in the Dockerfiles directory were modified and committed, or
# 2. "rebuild" param was passed as a first argument to this script.
#
# If the CI build is not of the "pull_request" type (i.e. in case of
# a pull request merge or any other "push" event) and it succeeds, the Docker image
# should be pushed to the ${CONTAINER_REG} repository.
# An empty file is created to signal that to next scripts.
#
# If the Docker image does not have to be rebuilt, it will be pulled from
# the ${CONTAINER_REG}. It can be also 'force' pulled if "pull" param was passed
# as a first argument to this script.
#

set -e

source $(dirname ${0})/set-ci-vars.sh

if [[ -z "${OS}" || -z "${OS_VER}" ]]; then
	echo "ERROR: The variables OS and OS_VER have to be set " \
		"(e.g. OS=fedora, OS_VER=32)."
	exit 1
fi

if [[ -z "${CONTAINER_REG}" ]]; then
	echo "ERROR: CONTAINER_REG environment variable is not set " \
		"(e.g. \"<registry_addr>/<org_name>/<package_name>\")."
	exit 1
fi

# Path to directory with Dockerfiles and image building scripts
images_dir_name=images
base_dir=utils/docker/${images_dir_name}

function build_image() {
	echo "Building the Docker image for the Dockerfile.${OS}-${OS_VER}"
	pushd ${images_dir_name}
	./build-image.sh
	popd
}

function pull_image() {
	echo "Pull the image from the Container Registry."
	docker pull ${CONTAINER_REG}:1.12-${OS}-${OS_VER}
}

# If "rebuild" or "pull" are passed to the script as param, force rebuild/pull.
if [[ "${1}" == "rebuild" ]]; then
	build_image
	exit 0
elif [[ "${1}" == "pull" ]]; then
	pull_image
	exit 0
fi


# Determine if we need to rebuild the image or just pull it from
# the Container Registry, based on commited changes.
if [ -n "${CI_COMMIT_RANGE}" ]; then
	commits=$(git rev-list ${CI_COMMIT_RANGE})
else
	commits=${CI_COMMIT}
fi

if [[ -z "${commits}" ]]; then
	echo "'commits' variable is empty. Docker image will be pulled."
fi

echo "Commits in the commit range:"
for commit in ${commits}; do echo ${commit}; done

echo "Files modified within the commit range:"
files=$(for commit in ${commits}; do git diff-tree --no-commit-id --name-only \
	-r ${commit}; done | sort -u)
for file in ${files}; do echo ${file}; done

# Check if committed file modifications require the Docker image to be rebuilt
for file in ${files}; do
	# Check if modified files are relevant to the current build
	if [[ ${file} =~ ^(${base_dir})\/Dockerfile\.(${OS})-(${OS_VER})$ ]] \
		|| [[ ${file} =~ ^(${base_dir})\/.*\.sh$ ]]
	then
		build_image

		# Check if the image has to be pushed to the Container Registry
		# (i.e. the build is triggered by commits to the ${GITHUB_REPO}
		# repository's stable-* or master branch, and the CI build is not
		# of the "pull_request" type). In that case, create the empty
		# file.
		if [[ "${CI_REPO_SLUG}" == "${GITHUB_REPO}" \
			&& (${CI_BRANCH} == stable-* || ${CI_BRANCH} == master) \
			&& ${CI_EVENT_TYPE} != "pull_request" \
			&& ${PUSH_IMAGE} == "1" ]]
		then
			echo "The image will be pushed to the Container Registry: ${CONTAINER_REG}"
			touch ${PUSH_IMAGE_FLAG}
		else
			echo "Skip pushing the image to the Container Registry."
		fi

		exit 0
	fi
done

# Getting here means rebuilding the Docker image is not required
pull_image
