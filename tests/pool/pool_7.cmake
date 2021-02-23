# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# test for creating too small pool
execute(${TEST_EXECUTABLE} c ${DIR}/testfile "test" 1 0600)

finish()
