# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2018, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# test existing file, file size >= min required size
#        layout matches the value from pool header
execute(${TEST_EXECUTABLE} c ${DIR}/testfile "test" 20 0600)
execute(${TEST_EXECUTABLE} o ${DIR}/testfile "test")

finish()
