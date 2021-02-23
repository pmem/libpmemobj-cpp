# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# test for creating pool, without enough memory available
execute(${TEST_EXECUTABLE} n ${DIR}/testfile "test" 20 0600)

finish()
