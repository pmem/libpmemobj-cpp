# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2019-2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# failed to open non-existent dir (and file)
execute(${TEST_EXECUTABLE} o ${DIR}/non-existent-dir/testfile "test")

finish()
