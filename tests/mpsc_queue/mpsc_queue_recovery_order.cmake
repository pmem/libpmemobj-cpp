# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

execute(${TEST_EXECUTABLE} ${DIR}/testfile 1)
execute(${TEST_EXECUTABLE} ${DIR}/testfile 0)

finish()
