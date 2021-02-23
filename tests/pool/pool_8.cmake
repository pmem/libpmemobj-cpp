# SPDX-License-Identifier: BSD-3-Clause
# Copyright 2021, Intel Corporation

include(${SRC_DIR}/../helpers.cmake)

setup()

# test for opening pool without access
execute(${TEST_EXECUTABLE} c ${DIR}/testfile "test" 20 0600)
execute_process(COMMAND chmod 000 ${DIR}/testfile)
execute(${TEST_EXECUTABLE} o ${DIR}/testfile "test")

finish()
