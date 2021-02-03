#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018-2021, Intel Corporation
#
set -e

# prepares input for ex-pman test
# usage: ./prepare_input.sh input_file

dd if=/dev/zero bs=64 count=1 2>>/dev/null >> ${1}
echo -n slkiiijjbjjii >> ${1}
dd if=/dev/zero bs=128 count=1 2>>/dev/null >> ${1}
echo -n q >> ${1}
