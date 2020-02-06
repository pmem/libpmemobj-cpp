# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# test for get_root on invalid pool handle
execute(${TEST_EXECUTABLE} i ${DIR}/testfile "test" 20 0600)

finish()
